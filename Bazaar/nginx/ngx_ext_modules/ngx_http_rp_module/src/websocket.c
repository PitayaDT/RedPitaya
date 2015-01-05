#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <libwebsockets.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_http_rp_module.h"

#include "websocket.h"

extern ngx_http_rp_module_ctx_t rp_module_ctx;

#define PARAM_LEN 50
#define SIG_NUM    5

pthread_t *sw_thread_handler = NULL;

static float **rp_signals = NULL;
char *buf;
float *fbuf;

int ws_stop;
struct ws_package_data *data;

int websocket_init(void){
    int ret = 0;

    sw_thread_handler = (pthread_t *)malloc(sizeof(pthread_t));
    if(sw_thread_handler == NULL) {
        return -1;
    }

    ws_stop = 0;
    ret = pthread_create(sw_thread_handler, NULL, ws_worker_thread, &ws_stop);
    if(ret != 0) {
        fprintf(stderr, "%s: pthread_create() failed: %s\n", __FUNCTION__,
                strerror(errno));
        return -1;
    }

    return 0;
}

int websocket_exit(void){
    int ret = 0;
    ws_stop = 1;
    if(sw_thread_handler) {
        ret = pthread_join(*sw_thread_handler, NULL);
        free(sw_thread_handler);
        sw_thread_handler = NULL;
    }
    if(ret != 0) {
        fprintf(stderr, "%s: pthread_join() failed: %s\n", __FUNCTION__,
                strerror(errno));
    }

    return 0;
}

/* We do not accept any http requests */
static int callback_http(struct libwebsocket_context *context, struct libwebsocket *wsi,
    enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len){

    return 0;
}

void rp_prepare_signals(float *fbuf){

    int rp_sig_num, rp_sig_len;
    int ret;
    int i, j;
    int k = 0;

    data->payload = 0;
    
    int retries = 200; //ms
    do{
        ret = rp_module_ctx.app.get_signals_func((float ***)&rp_signals, &rp_sig_num, &rp_sig_len); //get signals

        if(ret == -2)
            break;
        if(retries-- <= 0) {
            /* Use old signals */
            break;
        } else {
            usleep(1000);
        }

    }while (ret == -1);

    /* Populate arrayBuffer */
    //Overwrite sig_len with 50 -- TODO: Update to latest ws library for larger buff size.
    rp_sig_len = 50;
    /* X vector */
    for (i = 0; i < rp_sig_len; i++) {
        fbuf[k++] = rp_signals[0][i];
    }
    /* All the selected Y vectors follow -- we could add another parameter to determine how many signals do we need */
    for (i = 0; i < rp_sig_len; i++) {
        fbuf[k++] = rp_signals[1][i];
    }
    for (j = 0; j < 3; j++) {
        for (i = 0; i < rp_sig_len; i++) {
            fbuf[k++] = rp_signals[2][i];
        }
    }

    /* Init payload for the current ws packet */
    data->payload = k * sizeof(float); //How big is the body of the websocket packet.
}

static int rp_signals_callback(struct libwebsocket_context *context, 
                                       struct libwebsocket *wsi,
                                       enum libwebsocket_callback_reasons reason,
                                       void *user, void *in, size_t len){ // pointer to `void *in` holds the incomming request

    struct ws_package_data *data_usr = user;

    switch(reason) {
        case LWS_CALLBACK_ESTABLISHED: {// just log message that someone is connecting
            printf("connection established\n");
            break;
        }
        case LWS_CALLBACK_RECEIVE: {
            rp_prepare_signals(fbuf);
            
            unsigned char *fbp = (unsigned char *)fbuf;

            libwebsocket_write(wsi, fbp, data_usr->payload, LWS_WRITE_BINARY); //Binary type
        }
        case LWS_CALLBACK_CLOSED:
            fprintf(stderr, "DATA_CALLBACK_CLOSED\n");
            break;

        default:
            break;
    }
    return 0;
}

int align(size_t in, size_t size) {

    int ret = in;

    int remainder = in % size;
    if (remainder) {
        ret += size - remainder;
    }

    return ret;
}

/* Protocols used */
static struct libwebsocket_protocols protocols[] = {

    {
        "http_only",
        callback_http,
        0,
        0,
        NULL,
        0
    },
    {
        "rp_signals_callback_protocol",
        rp_signals_callback,
        sizeof(struct ws_package_data),
        0,
        NULL,
        0
    },
    {
        NULL, NULL, 0, 0, NULL, 0 //End of array
    }
};

void *ws_worker_thread(void *arg)
{
    fprintf(stderr, "Starting websocket server...\n");
    rp_websocket_main(arg);

    return 0;
}

int rp_websocket_main(int *stop){
    /* needed even with extensions disabled for create context */
    struct libwebsocket_context *context; 

    /*
    struct lws_context_creation_info {
        int port;
        const char *iface;
        struct libwebsocket_protocols *protocols;
        struct libwebsocket_extension *extensions;
        struct lws_token_limits *token_limits;
        const char *ssl_private_key_password;
        const char *ssl_cert_filepath;
        const char *ssl_private_key_filepath;
        const char *ssl_ca_filepath;
        const char *ssl_cipher_list;
        const char *http_proxy_address;
        unsigned int http_proxy_port;
        int gid;
        int uid;
        unsigned int options;
        void *user;
        int ka_time;
        int ka_probes;
        int ka_interval;
        #ifdef LWS_OPENSSL_SUPPORT
            SSL_CTX *provided_client_ssl_ctx;
        #else maintain structure layout either way 
                void *provided_client_ssl_ctx;
        #endif
    };
    */
    struct lws_context_creation_info info;

    int i;

    memset(&info, 0, sizeof info);
    info.port = 8080;
    info.protocols = protocols;

    #ifndef LWS_NO_EXTENSIONS
        info.extensions = libwebsocket_get_internal_extensions();
    #endif
    info.gid = -1;
    info.uid = -1;
    /* No special options */
    info.options = 0;

    // TODO: Use security algorithms
    info.ssl_cert_filepath = NULL;
    info.ssl_private_key_filepath = NULL;

    context = libwebsocket_create_context(&info);

    /* Error check */
    if(context == NULL) {
        fprintf(stderr, "libwebsocket init failed\n");
        return -1;
    }

     // Make sure the effective start of buffer is basic array type aligned
    int preamble = align(LWS_SEND_BUFFER_PRE_PADDING, sizeof(float));;

    /* Actual data malloc */
    int payload = (SIG_NUM) * sizeof(float);;
    /* Tail data  */
    /* Buffer for sending data */
    unsigned char *buf = (unsigned char*) malloc(preamble + payload + LWS_SEND_BUFFER_POST_PADDING);
    fbuf = (float *)&buf[preamble];

    rp_signals = (float **)malloc(SIG_NUM * sizeof(float *));
    for(i = 0; i < SIG_NUM; i++) {
        /* Fixed 2048 signal size */
        rp_signals[i] = (float *)malloc(50 * sizeof(float));
    }

    fprintf(stderr, "Websocket starting server...\n");

    int n = 0;
    while(!n && !(*stop)){

        /* https://libwebsockets.org/libwebsockets-api-doc.html */
        n = libwebsocket_service(context, 10);
    }

    libwebsocket_context_destroy(context);

    free(buf);

    for(i = 0; i < SIG_NUM; i++) {
        free(rp_signals[i]);
    }

    free(rp_signals);

    return 0;
}



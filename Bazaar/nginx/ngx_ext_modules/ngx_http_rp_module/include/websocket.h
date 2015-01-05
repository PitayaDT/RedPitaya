/**
 * $Id: worker.h 881 2013-12-16 05:37:34Z rp_jmenart $
 *
 * @brief Red Pitaya Oscilloscope worker.
 *
 * @Author Ales Bardorfer <ales.bardorfer@redpitaya.com>
 *         
 * (c) Red Pitaya  http://www.redpitaya.com
 *
 * This part of code is written in C programming language.
 * Please visit http://en.wikipedia.org/wiki/C_(programming_language)
 * for more details on the language used herein.
 */

#ifndef __WEBSOCKET_H
#define __WEBSOCKET_H


struct ws_package_data {
    int   preamble;
    int   payload;
    int   tail;
};

int websocket_init(void);
int websocket_exit(void);

void rp_prepare_signals(float *fbuf);
void *ws_worker_thread(void *args);
int   rp_websocket_main(int *stop);

#endif /* __WEBSOCKET_H */

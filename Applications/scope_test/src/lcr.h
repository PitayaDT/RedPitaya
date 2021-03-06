
#ifndef __LCR_H_INCLUDED
#define __LCR_H_INCLUDED

#include <stdint.h>
#include <complex.h> 
#include "generate.h"


#define M_PI 3.14159265358979323846



/** Signal types 
typedef enum {
	 Always sine
    eSignalSine_lcr,
             ///< Sinusoidal waveform.
    eSignalSquare,       ///< Square waveform.
    eSignalTriangle,     ///< Triangular waveform.
    eSignalSweep         ///< Sinusoidal frequency sweep.
} signal_e_lcr;

 //AWG FPGA parameters 
typedef struct {
    int32_t  offsgain;   ///< AWG offset & gain.
    uint32_t wrap;       ///< AWG buffer wrap value.
    uint32_t step;       ///< AWG step interval.
} awg_param_t_lcr;

*/
/* Forward declarations */

/* Functions for signal generation 
void synthesize_signal_lcr(double ampl, double freq, awg_signal_t type, double endfreq,
                       int32_t *data,
                       awg_param_t *params);
void write_data_fpga_lcr(uint32_t ch,
                     const int32_t *data,
                     const awg_param_t *awg);
*/

/* Data acquisition function */
int acquire_data(
                float **s , 
                uint32_t size);


/* New function added */
void write_data_fpga_lcr_single(uint32_t ch, const int32_t *data,
                     const awg_param_t *awg);



int LCR_data_analasys(float **s ,
                        uint32_t size,
                        uint32_t DC_bias,
                        uint32_t R_shunt,
                        float complex *Z,
                        double w_out,
                        int f);

/* LCR main function */

int lcr(uint32_t ch, double ampl, uint32_t DC_bias, float R_shunt, uint32_t averaging_num, int calib_function,  uint32_t Z_load_ref_real, uint32_t Z_load_ref_imag, 
        double steps, int sweep_function, double start_frequency, double end_frequency,  int scale_type, int wait_on_user, float *frequency_lcr, float *phase_lcr, float *amplitude_lcr);



/* Helper functions used in lcr main method */

float *create_table();

float *create_table_size(int num_of_el);

float **create_2D_table_size(int num_of_rows, int num_of_cols);

float max_array(float *arrayptr, int numofelements);

float mean_array(float *arrayptr, int numofelements);

float mean_array_column(float **arrayptr, int length, int column);


/*New section*/


float trapz(float *arrayptr, float T, int size1);

int get_gain(int *gain, const char *str);




/*New section*/

void usage();

int inquire_user_wait();

float run_lcr();

void lcr_acquire(float **rp_osc_signals);

int rp_osc_set_signals_lcr(float **source, int index);

/* Testing data acquisition */
//void simple_data_acquisition(float   **s);

#endif
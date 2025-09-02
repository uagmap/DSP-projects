#include "stdio.h"
#include "usbstk5515.h"
#include "aic3204.h"
#include "PLL.h"
#include "stereo.h"
#include "dsplib.h"
#include "usbstk5515_led.h"
#include "pushbuttons.h"

Int16 x_left;
Int16 x_right;
Int16 y_left;
Int16 y_right;

#define mod(x, m) (((x < 0) ? ((x % m) + m) : x) % m)

#define FILTER_LEN 15

DATA x[FILTER_LEN] = {0};
DATA d = 0;
DATA y = 0;
DATA e = 0;

DATA w[FILTER_LEN] = {0};
DATA delay_buffer[FILTER_LEN+2] = {0}; // delay buffer for fir() function
DATA mu = 3280;            // step size in Q1.15
int k = 0, x_index = 0;
long temp;

int switches=0, switches_1 = 0;
int noise_cancelling = 0;

#define SAMPLES_PER_SECOND 8000
#define GAIN_IN_dB 0
unsigned long int i = 0;

void main( void ) {
    USBSTK5515_init( );
    pll_frequency_setup(120);
    aic3204_hardware_init();
    aic3204_init();
    SAR_init();
    USBSTK5515_ULED_init();

    set_sampling_frequency_and_gain(SAMPLES_PER_SECOND, GAIN_IN_dB);

    for(k=0; k<FILTER_LEN+2; k++) delay_buffer[k] = 0;   // initialize delay buffer to zero

    rand16init();
    rand16(w, FILTER_LEN);        // initialize coeffs to random values

    for ( i = 0; i < SAMPLES_PER_SECOND * 600L; i++ ) {
        aic3204_codec_read(&x_left, &x_right);

        USBSTK5515_ULED_on(0);
        d          = x_left;       // audio + filtered noise on left channel from simulink
        x[x_index] = x_right;       // noise only from simulink

        fir(&x[x_index], w, &y, delay_buffer, 1, FILTER_LEN); // compute FIR filter output (y)

        e = d - y;             // compute error

        for(k=0; k<FILTER_LEN; k++) {
            temp = ( (long)mu*x[ mod(x_index-k, FILTER_LEN) ]) >> 14; // update coefficients
            temp = (long)e*temp >> 15;
            w[k] += (int)temp;
        }
        x_index++;                                  //take care of delay line index
        if(x_index >= FILTER_LEN) x_index = 0;
        USBSTK5515_ULED_off(0);

        if(noise_cancelling == 1) {
            y_left  = e; //filtered sound
            y_right = e;
        } else {
            y_left  = d;//with noise
            y_right = d;
        }

        aic3204_codec_write(y_left, y_right);

        if((switches_1==0) && (switches==1)) {        //if SW1 has been pressed
            noise_cancelling = !noise_cancelling;              //toggle variable noise_cancelling
            if(noise_cancelling==1)
                USBSTK5515_ULED_on(1);
            else
                USBSTK5515_ULED_off(1);
            }

            switches_1 = switches;
            switches = pushbuttons_read_raw();
        }

        aic3204_disable();
        printf( "\n***Program has Terminated***\n" );
        SW_BREAKPOINT;
}

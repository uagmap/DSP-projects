#include "stdio.h"
#include "usbstk5515.h"
#include "aic3204.h"
#include "PLL.h"
#include "stereo.h"
#include "dsplib.h"
#include "usbstk5515_led.h"
#include "pushbuttons.h"
#include "lab4_ir.h"

Int16 x_left;
Int16 x_right;
Int16 y_left;
Int16 y_right;
Int16 x, y;

#define SECTION_3

#define GAIN_IN_dB 6
unsigned long int i = 0;

DATA delay_buffer[H_LEN+2] = {0};
int switches = 0, switches_1 = 0;
int convolving = 0;

void main( void ) {
  USBSTK5515_init( );
  pll_frequency_setup(120);
  aic3204_hardware_init();
  aic3204_init();
  SAR_init();
  USBSTK5515_ULED_init();
  set_sampling_frequency_and_gain(FS, GAIN_IN_dB);
   
  for ( i = 0; i < FS * 600L; i++ ) {
    aic3204_codec_read(&x_left, &x_right);
    x = stereo_to_mono(x_left, x_right);
    
    USBSTK5515_ULED_on(0);
    fir(&x, h, &y, delay_buffer, 1, H_LEN); //FIR filter performing convolution
    USBSTK5515_ULED_off(0);

    #ifdef SECTION_3
        if(convolving == 1) {
            y_left  = y;
            y_right = y;
        } else {
            y_left  = x_left;
            y_right = x_right;
        }

        aic3204_codec_write(y_left, y_right);

        if((switches_1 != switches) && (switches == 1)) { // if variable switches changed
            convolving = !convolving;                    // and switch pressed is SW1
            if(convolving==1)                            // toggle variable convolving
                USBSTK5515_ULED_on(1);
            else
                USBSTK5515_ULED_off(1);
        }

        switches_1 = switches; //store previous value
        switches = pushbuttons_read_raw();
    #endif

  }

  aic3204_disable();
  printf( "\n***Program has Terminated***\n" );
  SW_BREAKPOINT;
}

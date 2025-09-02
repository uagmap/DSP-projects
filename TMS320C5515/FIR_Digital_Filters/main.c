#include "stdio.h"
#include "usbstk5515.h"
#include "aic3204.h"
#include "PLL.h"
#include "stereo.h"
#include "usbstk5515_led.h"
#include "pushbuttons.h"

#define mod(x,m) (((x < 0) ? ((x % m) + m) : x) % m)
#define FILTER_LEN 17
#define DELAY_LINE_LEN 64

Int16 x_left;
Int16 x_right;
Int16 y_left;
Int16 y_right;
Int16 x, x_delayed, y_lp, y_hp;
Int16 delay_line[DELAY_LINE_LEN] = {0};

int b_lp[FILTER_LEN] = {652, 659, 665, 671, 675, 678, 681, 682, 683, 682, 681, 678, 675, 671, 665, 659, 652};      // coefficients of low-pass filter
int b_hp[FILTER_LEN] = {-1130, -1439, -1738, -2015, -2258, -2458, -2608, -2700, 30038, -2700, -2608, -2458, -2258, -2015, -1738, -1439, -1129};    // coefficients of high-pass filter

long acc_lp = 0, acc_hp = 0;
int delay_line_index = 0, j;
char treble_enabled = 0, bass_enabled = 0;
unsigned int switches = 0, switches_1 = 0;
unsigned long int i = 0;

#define SAMPLES_PER_SECOND 48000     // sampling frequency in Hz
#define GAIN_IN_dB 0                  // ADC gain in dB

void main( void ) {
  USBSTK5515_init( );
  pll_frequency_setup(120);
  aic3204_hardware_init();
  aic3204_init();
  SAR_init();
  USBSTK5515_ULED_init();
  USBSTK5515_LED_off(0);

  printf( "<-> ECET 339 - Lab 10<->\n" );
  set_sampling_frequency_and_gain(SAMPLES_PER_SECOND, GAIN_IN_dB);
  
  for (i = 0; i < SAMPLES_PER_SECOND * 600L; i++) {
    aic3204_codec_read(&x_left, &x_right);
    x = stereo_to_mono(x_left, x_right);
    
    delay_line[delay_line_index] = x;   // push x[n] into the delay line


    // implement here low-pass filter
    acc_lp = 0;
    for(j=0; j<FILTER_LEN; j++) {
        x_delayed = delay_line[ mod(delay_line_index - j, DELAY_LINE_LEN) ];
        acc_lp += (long)x_delayed*b_lp[j]>>15; //multiply and accumulate
    }
    y_lp = (int)(acc_lp*4);


    // implement here high-pass filter
    acc_hp = 0;
       for(j=0; j<FILTER_LEN; j++) {
         x_delayed = delay_line[ mod(delay_line_index - j, DELAY_LINE_LEN) ];
         acc_hp += (long)x_delayed*b_hp[j]>>15; // multiply and accumulate
       }
    y_hp = (int)(acc_hp*4);



    y_left = 0;
    if(bass_enabled)
      y_left  = y_left + y_lp;
    if(treble_enabled)
      y_left  = y_left + y_hp;

    y_right = x_right;

    delay_line_index++;          // take care of delay_line_index
    if(delay_line_index >= DELAY_LINE_LEN) delay_line_index = 0;

    aic3204_codec_write(y_left, y_right);

    switches = pushbuttons_read_raw();
    if( switches == 1 && switches_1 == 0 ) { //SW1 has been pressed
       bass_enabled ^= 1;
       USBSTK5515_ULED_toggle(1);
    }
    if( switches == 2 && switches_1 == 0 ) { //SW2 has been pressed
      treble_enabled ^= 1;
      USBSTK5515_ULED_toggle(2);
    }
    if( switches == 3 )     //SW1 and SW2 have been pressed
      break;
    switches_1 = switches;

  }
  aic3204_disable();
  printf( "\n***Program has Terminated***\n" );
  SW_BREAKPOINT;
}

/*****************************************************************************/
/*                                                                           */
/* FILENAME                                                                  */
/* 	 main.c                                                                  */
/*                                                                           */
/* DESCRIPTION                                                               */
/*  Getting started with CCS and the TMS320C5515 USB Stick                   */
/*                                                                           */
/*****************************************************************************/

#include "stdio.h"
#include "usbstk5515.h"
#include "aic3204.h"
#include "PLL.h"
#include "stereo.h"
#include "sinewaves.h"

Int16 x_left;
Int16 x_right;
Int16 y_left;
Int16 y_right;
Int16 x, x_delayed, y, y_delayed;
Int16 x1_delayed, x2_delayed, x3_delayed, x4_delayed, x5_delayed, x6_delayed;
Int16 sinewave1;
int delay_line_index = 0;
long temp;

#define SAMPLES_PER_SECOND 8000      // sampling frequency in Hz
#define GAIN_IN_dB 36                 // ADC gain in dB      
unsigned long int i = 0;


#define mod(x, m) (((x < 0) ? ((x%m) + m) : x) % m)

//#define SECTION_2
//#define SECTION_3
#define SECTION_5

#ifdef SECTION_2
    #define DELAY_LINE_LEN 8000
    #define DELAY 4000 //500ms
Int16 delay_line[DELAY_LINE_LEN] = {0};
#endif

#ifdef SECTION_3
    #define DELAY_LINE_LEN 8000
    Int16 delay_line[DELAY_LINE_LEN] = {0};

    Int16 c1 = 13107;
    Int16 c2 = 2327;
    Int16 c3 = 6914;
    Int16 c4 = 15008;
    Int16 c5 = 12976;
    Int16 c6 = 15696;

    #define DELAY1 400
    #define DELAY2 800
    #define DELAY3 1200
    #define DELAY4 1600
    #define DELAY5 2000
    #define DELAY6 2400
#endif

#ifdef SECTION_4
    Int16 A = 40;
    Int16 B = 80;
    #define DELAY_LINE_LEN 8000
    Int16 delay_line[DELAY_LINE_LEN] = {0};
    Int16 delay;
#endif

#ifdef SECTION_5
    #define DELAY_LINE_LEN 8000
    Int16 delay_line[DELAY_LINE_LEN] = {0};
    Int16 a = 24576;
    Int16 g = 3277;
    Int16 L = 4000;
#endif

/* ------------------------------------------------------------------------ *
 *                                                                          *
 *  main( )                                                                 *
 *                                                                          *
 * ------------------------------------------------------------------------ */
void main( void ) 
{
  /* Initialize BSL */
  USBSTK5515_init( );		  	  // implemented in usbstk5515.c
	
  /* Initialize PLL */
  pll_frequency_setup(100);      // implemented in PLL.c

  /* Initialise hardware interface and I2C for code */
  aic3204_hardware_init();	     // implemented in aic3204.c
    
  /* Initialise the AIC3204 codec */
  aic3204_init(); 		         // implemented in aic3204_init.c

  /* Descriptive text that will appear in the console */ 
  printf( "<-> ECET 339 <->\n" );
	
  /* Setup sampling frequency and 30dB gain for microphone */
  set_sampling_frequency_and_gain(SAMPLES_PER_SECOND, GAIN_IN_dB);   // implemented in aic3204_init.c
  
  for ( i = 0; i < SAMPLES_PER_SECOND * 600L; i++ ) {
    /* read left and right channels from codec	*/ 		
    aic3204_codec_read(&x_left, &x_right);         // implemented in aic3204.c
   
    /* converts stereo signal to mono signal */
    x = stereo_to_mono(x_left, x_right);  		   // implemented in stereo.c
   
#ifdef SECTION_1 //ring modulation
    sinewave1 = generate_sinewave_1(500, 4587, 8000);//target f, ampl, sampling f
    temp = ((long)x*sinewave1>>15);
    y_left = (int)temp;
    y_right = y_left;
#endif
    
#ifdef SECTION_2 //echo effect
    delay_line[delay_line_index] = x;
    x_delayed = delay_line[mod(delay_line_index - DELAY, DELAY_LINE_LEN)];
    temp = ((long)x_delayed*24576>>15);

    y_left = x + (int)temp;
    y_right = y_left;

    delay_line_index++;
    if(delay_line_index >= DELAY_LINE_LEN) delay_line_index = 0;
#endif

#ifdef SECTION_3 //chorus effect
    delay_line[delay_line_index] = x; // push x[n] into delay line
    x1_delayed = delay_line[ mod(delay_line_index - DELAY1, DELAY_LINE_LEN) ];
    x2_delayed = delay_line[ mod(delay_line_index - DELAY2, DELAY_LINE_LEN) ];
    x3_delayed = delay_line[ mod(delay_line_index - DELAY3, DELAY_LINE_LEN) ];
    x4_delayed = delay_line[ mod(delay_line_index - DELAY4, DELAY_LINE_LEN) ];
    x5_delayed = delay_line[ mod(delay_line_index - DELAY5, DELAY_LINE_LEN) ];
    x6_delayed = delay_line[ mod(delay_line_index - DELAY6, DELAY_LINE_LEN) ];
    delay_line_index++;

    temp  = ((long)x1_delayed*c1>>15) + ((long)x2_delayed*c2>>15) +
    ((long)x3_delayed*c3>>15) + ((long)x4_delayed*c4>>15) +
    ((long)x5_delayed*c5>>15) + ((long)x6_delayed*c6>>15);

    y_left  = x + (int)temp;
    y_right = y_left;

    if(delay_line_index >= DELAY_LINE_LEN) delay_line_index = 0;
#endif

#ifdef SECTION_4 //vibrato effect
    delay_line[delay_line_index] = x;
    // push x[n] into delay line
    delay     = generate_sinewave_1(5, A, SAMPLES_PER_SECOND) + B;
    x_delayed = delay_line[ mod(delay_line_index - delay, DELAY_LINE_LEN)];

    y_left    = x_delayed;
    y_right   = y_left;

    delay_line_index++;
    // take care of delay line index
    if(delay_line_index >= DELAY_LINE_LEN) delay_line_index = 0;
#endif

#ifdef SECTION_5  //reverberation
    y_delayed = delay_line[ mod(delay_line_index - L, DELAY_LINE_LEN) ];
    temp = ((long)x*a>>15) + ((long)y_delayed*g>>15);
    y    = (int)temp;

    delay_line[delay_line_index] = y;
    y_left    = y;
    y_right   = y_left;

    delay_line_index++;
    if(delay_line_index >= DELAY_LINE_LEN) delay_line_index = 0;
#endif

    /* write left and rght channels to codec */
    aic3204_codec_write(y_left, y_right);         // implemented in aic3204.c
  }

  /* Disable I2S and put codec into reset */ 
  aic3204_disable();

  printf( "\n***Program has Terminated***\n" );
  SW_BREAKPOINT;
}


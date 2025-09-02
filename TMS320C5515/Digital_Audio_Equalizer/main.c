#include "stdio.h"
#include "usbstk5515.h"
#include "aic3204.h"
#include "PLL.h"
#include "stereo.h"
#include "dsplib.h"
#include "usbstk5515_led.h"
#include "pushbuttons.h"
#include "oled.h"
#include "iir_coefficients.h" //this has coefficients for all bands

Int16 x_left;
Int16 x_right;
Int16 y_left;
Int16 y_right;

#define N 16 //filter block size
#define GAIN_IN_dB 0
unsigned long int i = 0;

#define mod(x, m) (((x < 0) ? ((x % m) + m) : x) % m)

DATA x[2*N] = {0};
DATA y[2*N] = {0};
DATA y1[2*N] = {0};
DATA y2[2*N] = {0};
DATA y3[2*N] = {0};
DATA y4[2*N] = {0};
DATA y5[2*N] = {0};
DATA y6[2*N] = {0};
DATA y7[2*N] = {0};
DATA y8[2*N] = {0};

DATA buffer1[4*NUM_SOS+1] = {0};
DATA buffer2[4*NUM_SOS+1] = {0};
DATA buffer3[4*NUM_SOS+1] = {0};
DATA buffer4[4*NUM_SOS+1] = {0};
DATA buffer5[4*NUM_SOS+1] = {0};
DATA buffer6[4*NUM_SOS+1] = {0};
DATA buffer7[4*NUM_SOS+1] = {0};
DATA buffer8[4*NUM_SOS+1] = {0};

int oflag;
int k = 0;
int sign = 1;
int gain_preset = 0;
int switches = 0, switches_1 = 0;
int oled_lines[48] = {0};
unsigned int band = 0;
int gain;
int height;
unsigned int j = 0;
long temp = 0;

int gains[4][8] = { //define gain presets
                   {2047, 2047, 2047, 2047, 2047, 2047, 2047, 2047}, //flat
                   {2047, 1928, 1512, 1023, 1023, 1512, 1928, 2047},
                   {511, 725, 1023, 2047, 2047, 1023, 725, 512}, //hip hop
                   {0, 0, 0, 0, 0, 0, 0, 0}                         //mute
};

// function declaration – actual implementation in iircas51_q214.asm
ushort iircas51_q214(DATA *, DATA *, DATA *, DATA *, ushort, ushort);

void main( void ) {
  USBSTK5515_init( );
  pll_frequency_setup(120);
  aic3204_hardware_init();
  aic3204_init();
  SAR_init();
  USBSTK5515_ULED_init();
  oled_init();
  set_sampling_frequency_and_gain(FS, GAIN_IN_dB);

  USBSTK5515_ULED_on(0);

  for (i = 0; i < 4*NUM_SOS+1; i++) buffer1[i] = 0; //initialize to 0
  for (i = 0; i < 4*NUM_SOS+1; i++) buffer2[i] = 0; //initialize to 0
  for (i = 0; i < 4*NUM_SOS+1; i++) buffer3[i] = 0; //initialize to 0
  for (i = 0; i < 4*NUM_SOS+1; i++) buffer4[i] = 0; //initialize to 0
  for (i = 0; i < 4*NUM_SOS+1; i++) buffer5[i] = 0; //initialize to 0
  for (i = 0; i < 4*NUM_SOS+1; i++) buffer6[i] = 0; //initialize to 0
  for (i = 0; i < 4*NUM_SOS+1; i++) buffer7[i] = 0; //initialize to 0
  for (i = 0; i < 4*NUM_SOS+1; i++) buffer8[i] = 0; //initialize to 0

  for ( i = 0; i < FS * 600L; i++ ) { //runs for 10 minutes
    aic3204_codec_read(&x_left, &x_right);
    x[k] = stereo_to_mono(x_left, x_right);


    y_left = y[k+sign*N]; //play previous output block
    y_right = y[k+sign*N]; //play previous input block
    k++;

    //band filters
    if(k == 2*N) {
        oflag = iircas51_q214(&x[N], Hlow, &y1[N], buffer1, NUM_SOS, N);
        oflag = iircas51_q214(&x[N], H1, &y2[N], buffer2, NUM_SOS, N);
        oflag = iircas51_q214(&x[N], H2, &y3[N], buffer3, NUM_SOS, N);
        oflag = iircas51_q214(&x[N], H3, &y4[N], buffer4, NUM_SOS, N);
        oflag = iircas51_q214(&x[N], H4, &y5[N], buffer5, NUM_SOS, N);
        oflag = iircas51_q214(&x[N], H5, &y6[N], buffer6, NUM_SOS, N);
        oflag = iircas51_q214(&x[N], H6, &y7[N], buffer7, NUM_SOS, N);
        oflag = iircas51_q214(&x[N], Hhigh, &y8[N], buffer8, NUM_SOS, N);
        sign *= -1;
        k = 0;

        //now sum the outputs in one signal
        for(j=N; j<2*N; j++)
        {
            temp = (long)gains[gain_preset][0]*(long)y1[j] + (long)gains[gain_preset][1]*(long)y2[j] + (long)gains[gain_preset][2]*(long)y3[j] + (long)gains[gain_preset][3]*(long)y4[j] + (long)gains[gain_preset][4]*(long)y5[j] + (long)gains[gain_preset][5]*(long)y6[j] + (long)gains[gain_preset][6]*(long)y7[j] + (long)gains[gain_preset][7]*(long)y8[j];

            y[j] = (int)(temp >> 14);
        }
    }
    

    if(k == N) {
        oflag = iircas51_q214(&x[0], Hlow, &y1[0], buffer1, NUM_SOS, N);
        oflag = iircas51_q214(&x[0], H1, &y2[0], buffer2, NUM_SOS, N);
        oflag = iircas51_q214(&x[0], H2, &y3[0], buffer3, NUM_SOS, N);
        oflag = iircas51_q214(&x[0], H3, &y4[0], buffer4, NUM_SOS, N);
        oflag = iircas51_q214(&x[0], H4, &y5[0], buffer5, NUM_SOS, N);
        oflag = iircas51_q214(&x[0], H5, &y6[0], buffer6, NUM_SOS, N);
        oflag = iircas51_q214(&x[0], H6, &y7[0], buffer7, NUM_SOS, N);
        oflag = iircas51_q214(&x[0], Hhigh, &y8[0], buffer8, NUM_SOS, N);
        sign *= -1;

        for(j=0; j<N; j++)
        {
            temp = (long)gains[gain_preset][0]*(long)y1[j] + (long)gains[gain_preset][1]*(long)y2[j] + (long)gains[gain_preset][2]*(long)y3[j] + (long)gains[gain_preset][3]*(long)y4[j] + (long)gains[gain_preset][4]*(long)y5[j] + (long)gains[gain_preset][5]*(long)y6[j] + (long)gains[gain_preset][6]*(long)y7[j] + (long)gains[gain_preset][7]*(long)y8[j];

            y[j] = (int)(temp >> 14);
        }
    }



    if((switches_1 == 0) && (switches == 1)) { // if variable switches changed
        gain_preset++;
        gain_preset = mod(gain_preset, 4); //cycle the presets
        oled_display_message("                 ", "                   "); //clear screen on button press

        if (gain_preset == 0) {
            USBSTK5515_ULED_on(0);
            USBSTK5515_ULED_off(1);
            USBSTK5515_ULED_off(2);
            USBSTK5515_ULED_off(3);
        } else if (gain_preset == 1) {
            USBSTK5515_ULED_off(0);
            USBSTK5515_ULED_on(1);
            USBSTK5515_ULED_off(2);
            USBSTK5515_ULED_off(3);
        } else if (gain_preset == 2) {
            USBSTK5515_ULED_off(0);
            USBSTK5515_ULED_off(1);
            USBSTK5515_ULED_on(2);
            USBSTK5515_ULED_off(3);
        } else if (gain_preset == 3) {
            USBSTK5515_ULED_off(0);
            USBSTK5515_ULED_off(1);
            USBSTK5515_ULED_off(2);
            USBSTK5515_ULED_on(3);
        }

        // Update OLED display
            for (band = 0; band < 8; band++) {
                gain = gains[gain_preset][band];
                height = gain * 16; // Scale to 0-32767
                if (height < 0) height = 1;
                else if (height > 32767) height = 32767;

                // Update 6 lines per band
                for (j = 0; j < 5; j++) {
                    oled_lines[band*6 + j] = height;
                }

                //add space between bands
                for (j=5; j<6; j++) {
                    oled_lines[band*6 + j] = 0; //0 for blank spacing line
                }
            }
            oled_display_lines(&oled_lines[0]); // Refresh display
    }

    switches_1 = switches; //store previous value
    switches = pushbuttons_read_raw();





    aic3204_codec_write(y_left, y_right);
  }

  aic3204_disable();
  printf( "\n***Program has Terminated***\n" );
  SW_BREAKPOINT;
}

/*
  $Header: /root/Signalogic/DirectCore/lib/alglib/agc.c
 
  Description: AGC algorithms
 
  Projects: SigSRF, DirectCore
 
  Copyright Signalogic Inc. 2018

  Revision History:
  
   Created Jul 2018 Jeff Brower
*/

#include <math.h>

/* DSAgc() -- in-place saturation control (form of Automatic Gain Control)

  input:   floating-point (single precision) array of size n
  output:  short int array of size n

  Sections of this source Copyright EVS Codec 3GPP TS26.443 Jun 30, 2015. Version CR 26.443-0006
  http://www.3gpp.org/ftp/tsg_sa/WG4_CODEC/EVS_Testing/CR26443-0006-ANSI-C_source_code/c-code/lib_dec/syn_outp.c
*/


int DSAgc(
         float x[],     /* i/o: input/output vector */
         float mem[],   /* per channel memory values array of size 2, init to [0,0] */
         const short N  /* vector size, in number of elements */
         ) {

int i;
float fac, prev, tmp;
float max = 0, frame_fac = 0;

/* calculate and adjust gain factor to avoid saturation */

   for (i=0; i<N; i++) if ((tmp = (float)fabs(x[i])) > max) max = tmp;

   if ( max > 30000.0f ) frame_fac = 0.5f - (15000.0f/max);  /* assuming 32767 max value for output values */

   fac = mem[0];
   prev = mem[1];

/* apply current gain to signal and update the gain value */

   for (i=0; i<N; i++) {

   /* update gain factor using 1-pole LPF filter (exponential response) */

      fac = 0.1f*frame_fac + 0.9f*fac;

   /* apply gain to input */

      tmp = (1.0f - fac)*x[i] - fac*prev;
      prev = x[i];

   /* clip to max allowable +/- short int values */ 

      if (tmp > 32767.0f) tmp = 32767.0f;
      else if (tmp < -32768.0f) tmp = -32768.0f;

      x[i] = (short)floor(tmp + 0.5f);  /* store output as short int's */
   }

   mem[0] = fac;  /* update per-channel mem values */
   mem[1] = prev;

   return i;  /* return number of elements processed */
}


#if 0  /* currently not used, only here for energy calculation reference */

/******************************************************************************
**
** Function             : AGC()
**
** Description          : AGC algorithm is used to automatically adjust the 
**                        speech level of an audio signal to a predetermined 
**                        value noramally in db.
**
** Arguments:
**  float *x            : input vector (range from -1 to 1 )
**  float *y            : output vector (range from -1 to 1 )
**  float gain_level    : output power level in db
**  int   N             : number of samples or frame length
**
** Inputs:
**  float *x             
**  float gain_level    
**  int   N             
**
** Outputs:
**  float *y            
**
** Return value         : None
**
** NOTE                 : For more details refer matlab files. 
**
** Programmer           : Jaydeep Appasaheb Dhole
**                      : Associate Software Engineer ( DSP )
**                      : Aparoksha Tech. Pvt. Ltd. ,Bangalore.
**                      : http://www.aparoksha.com
**                      : <jaydeepdhole@gmail.com>
**
** Date                 : 26 May 2006.
******************************************************************************/

#include<math.h>

void  AGC(float *x, float *y, float gain_level, int N)
{
    int i;
    float energy, output_power_normal, K;
    
    /* ouput power gain level is in db convert it into normal power */
    output_power_normal = (float)pow((double)10,(double)(gain_level/10));

    /* Calculate the energy of the signal */
    energy = 0;
    for(i = 0; i < N; i++)
        energy += x[i] * x[i];

    /* calculate the multiplication factor */
    K = (float)sqrt ((output_power_normal*N)/energy);

    /* scale the input signal to achieve the required output power */
    for(i = 0; i < N; i++)
        y[i] = x[i] * K ;
}

#endif

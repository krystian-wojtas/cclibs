/*---------------------------------------------------------------------------------------------------------*\
  File:     meas.c                                                                      Copyright CERN 2014

  License:  This file is part of libreg.

            libreg is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  This provides measurement related functions for libreg.
\*---------------------------------------------------------------------------------------------------------*/

#include <string.h>
#include "libreg.h"

#define STOP_FILTER     0xFFFFFFFF

/*---------------------------------------------------------------------------------------------------------*/
static float regMeasFirFilter(struct reg_meas_filter *filter)
/*---------------------------------------------------------------------------------------------------------*\
  This function implements the two-stage box-car FIR filter used by regMeasFilter() and
  regMeasFilterInitHistory().
\*---------------------------------------------------------------------------------------------------------*/
{
    int32_t input_integer;

    // Filter stage 1

    input_integer = (int32_t)(filter->float_to_integer * filter->unfiltered);

    filter->fir_accumulator[0] += (input_integer - filter->fir_buf[0][filter->fir_index[0]]);

    filter->fir_buf[0][filter->fir_index[0]] = input_integer;

    if(++filter->fir_index[0] >= filter->fir_length[0])
    {
        filter->fir_index[0] = 0;
    }

    // Filter stage 2

    input_integer = filter->fir_accumulator[0] / filter->fir_length[0];

    filter->fir_accumulator[1] += (input_integer - filter->fir_buf[1][filter->fir_index[1]]);

    filter->fir_buf[1][filter->fir_index[1]] = input_integer;

    if(++filter->fir_index[1] >= filter->fir_length[1])
    {
        filter->fir_index[1] = 0;
    }

    // Convert filter output back to floating point

    return(filter->integer_to_float * (float)filter->fir_accumulator[1]);
}
/*---------------------------------------------------------------------------------------------------------*/
void regMeasFilter(struct reg_meas_filter *filter)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called in real-time to filter the measurement with a 2-stage cascaded box car filter and
  then uses extrapolation to estimate the measurement without the measurement and filtering delay.
  If the filter is not running then the output is simply the unfiltered input.
\*---------------------------------------------------------------------------------------------------------*/
{
    float   old_filtered_value;
    
    // If filter is stopped
    
    if(filter->stop_iters == STOP_FILTER)
    {
        // Bypass the filter - simply set the output values to the input value
        
        filter->extrapolated = filter->filtered = filter->unfiltered;
    }
    else // Filter is starting or running
    {
        filter->filtered = regMeasFirFilter(filter);

        // Extrapolate to estimate the measurement without a delay

        old_filtered_value = filter->extrapolation_buf[filter->extrapolation_index];

        filter->extrapolation_buf[filter->extrapolation_index] = filter->filtered;

        if(++filter->extrapolation_index >= filter->extrapolation_len_iters)
        {
            filter->extrapolation_index = 0;
        }

        filter->extrapolated = filter->filtered;

        // If filter is starting

        if(filter->stop_iters > 0)
        {
            filter->stop_iters--;
        }
        else // else filter is running
        {
            filter->extrapolated += (filter->filtered - old_filtered_value) * filter->extrapolation_factor;
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void regMeasFilterInitBuffer(struct reg_meas_filter *filter, int32_t *buf)
/*---------------------------------------------------------------------------------------------------------*\
  This function allows the buffer for the measurement filter to be defined.  The buffer is used for both
  FIR filter stages and the extrapolation history, so it needs to be long enough to cover all three
  requirements: fir_length[0] + fir_length[1] + extrapolation_len_iters
\*---------------------------------------------------------------------------------------------------------*/
{
    filter->fir_buf[0] = buf;
}
/*---------------------------------------------------------------------------------------------------------*/
void regMeasFilterInit(struct reg_meas_filter *filter, uint32_t fir_length[2],
                       uint32_t extrapolation_len_iters, float meas_delay_iters)
/*---------------------------------------------------------------------------------------------------------*\
  This function initialises the FIR measurement filter.  It will also initialise the filter
  delays and the extrapolation factors to cancel these delays. If max value has already been given by
  calling regMeasFilterInitMax()I then the filter history will initialised using filter->unfiltered
  and filter will be started.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Stop the filter
    
    filter->stop_iters = STOP_FILTER;
    
    // Save filter parameters
    
    filter->fir_length[0]           = fir_length[0];
    filter->fir_length[1]           = fir_length[1];
    filter->extrapolation_len_iters = extrapolation_len_iters;
    filter->meas_delay_iters        = meas_delay_iters;

    // Set the pointers to the second stage FIR buffer and extrapolation buffer
    
    filter->fir_buf[1] = filter->fir_buf[0] + fir_length[0];
    filter->extrapolation_buf = (float*)filter->fir_buf[1] + fir_length[1];
    
    // Calculate FIR filter delay and extrapolation factor
    
    filter->fir_delay_iters  = 0.5 * (float)(fir_length[0] + fir_length[1]) - 1.0;
        
    filter->extrapolation_factor = (float)(meas_delay_iters + filter->fir_delay_iters) /
                                   (float)extrapolation_len_iters;

    // If max value already defined then reset, initialise and restart the filter
    
    if(filter->max_value > 0.0)
    {
        regMeasFilterInitHistory(filter);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void regMeasFilterInitMax(struct reg_meas_filter *filter, float pos, float neg)
/*---------------------------------------------------------------------------------------------------------*\
  This function calculates the scale factors for the FIR filters from the pos/neg limits for the
  measurement being filtered. This is necessary because the filter must work with integers to avoid
  rounding errors. If the filter lengths have been given by regMeasFilterInit() then the filter
  will be reinitialised using filter->unfiltered and started.
\*---------------------------------------------------------------------------------------------------------*/
{
    float max;
    
    // Stop filter
    
    filter->stop_iters = STOP_FILTER;
    
    // Calculate max from pos and neg limits and allow 20% over-range
    
    max = 1.2 * (pos > -neg ? pos : -neg);
    
    // Calculate float to integer scaling
    
    filter->max_value        = max;
    filter->float_to_integer = INT32_MAX / (filter->fir_length[0] * max);
    filter->integer_to_float = max / INT32_MAX;
    
    // If the filter orders have already been defined then reset, initialise and restart the filter
    
    if(filter->fir_length[0] > 0)
    {
        regMeasFilterInitHistory(filter);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void regMeasFilterInitHistory(struct reg_meas_filter *filter)
/*---------------------------------------------------------------------------------------------------------*\
  This function will initialise the filter history using filter->unfiltered, and will restart the filter.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t i;
    uint32_t len;
    
    // Stop filter
    
    filter->stop_iters = STOP_FILTER;
    
    // Reset the FIP filters

    filter->fir_accumulator[0] = filter->fir_accumulator[1] = 0;
    
    memset(filter->fir_buf[0],0,(filter->fir_length[0] + filter->fir_length[1]) * sizeof(int32_t));

    // Initialise filter to the value in filter->unfiltered
    
    len = filter->fir_length[0] + filter->fir_length[1];
    
    for(i = 0 ; i < len ; i++)
    {
        regMeasFirFilter(filter);
    }

    // Restart filter with a delay to allow the extrapolation buffer to fill

    filter->stop_iters = filter->extrapolation_len_iters;
}
/*---------------------------------------------------------------------------------------------------------*/
void regMeasRegSelect(struct reg_meas_filter *filter, enum reg_meas_select reg_select)
/*---------------------------------------------------------------------------------------------------------*\
  This function sets the selector of the regulation measurement.
\*---------------------------------------------------------------------------------------------------------*/
{
    filter->reg_select = reg_select;
}
/*---------------------------------------------------------------------------------------------------------*/
float regMeasReg(struct reg_meas_filter *filter)
/*---------------------------------------------------------------------------------------------------------*\
  This function returns the measurement selected by calling regMeasRegSelect().
\*---------------------------------------------------------------------------------------------------------*/
{
    switch(filter->reg_select)
    {
        case REG_MEAS_UNFILTERED:   return(filter->unfiltered);
        case REG_MEAS_FILTERED:     return(filter->filtered);
        case REG_MEAS_EXTRAPOLATED: return(filter->extrapolated);
    }

    return(0.0);
}
/*---------------------------------------------------------------------------------------------------------*/
void regMeasSetNoiseAndTone(struct reg_noise_and_tone *noise_and_tone, float noise_pp,
                            float tone_amp, uint32_t tone_half_period_iters)
/*---------------------------------------------------------------------------------------------------------*\
  This function will set the noise and tone characteristics for a simulated measurement.
\*---------------------------------------------------------------------------------------------------------*/
{
    noise_and_tone->noise_pp = noise_pp;
    noise_and_tone->tone_amp = tone_amp;
    noise_and_tone->tone_half_period_iters = tone_half_period_iters;
}
/*---------------------------------------------------------------------------------------------------------*/
float regMeasNoiseAndTone(struct reg_noise_and_tone *noise_and_tone)
/*---------------------------------------------------------------------------------------------------------*\
  This function uses a simple pseudo random number generator to generate a roughly white noise and a
  square wave to simulate a tone. The frequency of the tone is defined by its half-period in iterations.
\*---------------------------------------------------------------------------------------------------------*/
{
    float            noise;                                 // Roughly white noise
    float            tone;                                  // Square wave tone
    static uint32_t  noise_random_generator = 0x8E35B19C;   // Use fixed initial seed

    // Use efficient random number generator to calculate the roughly white noise

    if(noise_and_tone->noise_pp != 0.0)
    {
        noise_random_generator = (noise_random_generator << 16) +
                               (((noise_random_generator >> 12) ^ (noise_random_generator >> 15)) & 0x0000FFFF);

        noise = noise_and_tone->noise_pp * (float)((int32_t)noise_random_generator) / 4294967296.0;
    }
    else
    {
        noise = 0.0;
    }

    // Use efficient square tone generator to create tone

    if(noise_and_tone->tone_amp != 0.0)
    {
        if(++noise_and_tone->iter_counter >= noise_and_tone->tone_half_period_iters)
        {
            noise_and_tone->tone_toggle  = !noise_and_tone->tone_toggle;
            noise_and_tone->iter_counter = 0;
        }

        tone = noise_and_tone->tone_toggle ? noise_and_tone->tone_amp : -noise_and_tone->tone_amp;
    }
    else
    {
        tone = 0.0;
    }

    // Return sum of noise and tone

    return(noise + tone);
}
/*---------------------------------------------------------------------------------------------------------*/
void regMeasRateStore(struct reg_meas_rate *meas_rate, float meas, int32_t period_iters)
/*---------------------------------------------------------------------------------------------------------*\
  This function will store the filtered measurement in the rate estimation history at the reg_period
\*---------------------------------------------------------------------------------------------------------*/
{
    // Store measurement at the specified period

    if(++meas_rate->iter_counter >= period_iters)
    {
        meas_rate->iter_counter = 0;
        meas_rate->buf_index    = (meas_rate->buf_index + 1) & REG_MEAS_RATE_BUF_MASK;

        meas_rate->buf[meas_rate->buf_index] = meas;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
float regMeasRate(struct reg_meas_rate *meas_rate, float period)
/*---------------------------------------------------------------------------------------------------------*\
  This function will use linear regression through four points to estimate the rate of change of the
  measurement stored by regMeasRateStore().
\*---------------------------------------------------------------------------------------------------------*/
{
    float    *buf   = meas_rate->buf;           // Local pointer to history buffer
    uint32_t  index = meas_rate->buf_index;     // Local copy of index of most recent sample

    // Least-squares linear regression through four points

    meas_rate->rate = 2.0 / (20.0 * period) * (3.0 * (buf[ index ] -
                                                      buf[(index - 3) & REG_MEAS_RATE_BUF_MASK]) +
                                                     (buf[(index - 1) & REG_MEAS_RATE_BUF_MASK]  -
                                                      buf[(index - 2) & REG_MEAS_RATE_BUF_MASK]));

    return(meas_rate->rate);
}
// EOF

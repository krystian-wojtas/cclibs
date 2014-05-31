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

static float regMeasFirFilterRT(struct reg_meas_filter *filter);

//-----------------------------------------------------------------------------------------------------------
// Non-Real-Time Functions - do not call these from the real-time thread or interrupt
//-----------------------------------------------------------------------------------------------------------
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
                       uint32_t extrapolation_len_iters, float pos, float neg, float meas_delay_iters)
/*---------------------------------------------------------------------------------------------------------*\
  This function initialises the FIR measurement filter.  It needs the lengths of the FIR filter stages
  and the extrapolation buffer length. It also needs to calculate scale factors for the FIR filters
  from the pos/neg limits for the measurement being filtered. This is necessary because the filter must
  work with integers to avoid rounding errors. The meas_delay_iters parameter must define the delay in
  the "unfiltered" measurement - this is the hardware filtering delay.

  The filter history buffers will be initialised using filter->signal[REG_MEAS_UNFILTERED] as the input
  and then it will be restarted. This can be done at background level while the filter is being called
  in real-time, but the filter will be bypassed during the initialisation process, so expect a perturbation
  and potential instability.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    total_fir_len;
    uint32_t    longest_fir_len;
    float       max_meas_value;
    float      *extrapolation_buf;

    // Stop the filter
    
    filter->enable = 0;

    // Calculate important filter variables

    total_fir_len   = fir_length[0] + fir_length[1];
    longest_fir_len = fir_length[0] > fir_length[1] ? fir_length[0] : fir_length[1];
    max_meas_value  = 1.1 * (pos > -neg ? pos : -neg);

    // Save filter parameters
    
    filter->fir_length[0]           = fir_length[0];
    filter->fir_length[1]           = fir_length[1];
    filter->extrapolation_len_iters = extrapolation_len_iters;
    filter->max_meas_value          = max_meas_value;

    // Set the pointers to the second stage FIR buffer and extrapolation buffer
    
    filter->fir_buf[1]        = filter->fir_buf[0] + fir_length[0];
    filter->extrapolation_buf = extrapolation_buf = (float*)(filter->fir_buf[1] + fir_length[1]);

    // Set measurement delays
    
    filter->delay_iters[REG_MEAS_UNFILTERED]   = meas_delay_iters;
    filter->delay_iters[REG_MEAS_FILTERED]     = meas_delay_iters + 0.5 * (float)(total_fir_len - 2);
    filter->delay_iters[REG_MEAS_EXTRAPOLATED] = 0.0;

    // Calculate extrapolation factor so that it cancels the filtered measurement delay

    filter->extrapolation_factor = filter->delay_iters[REG_MEAS_FILTERED] / (float)extrapolation_len_iters;
    
    // Calculate float/integer scalings for FIR filter stages

    filter->float_to_integer = INT32_MAX / (longest_fir_len * max_meas_value);
    filter->integer_to_float = 1.0 / (filter->float_to_integer * (float)(total_fir_len - longest_fir_len));
    
    // Initialise the FIR filter stages
    
    filter->fir_accumulator[0] = filter->fir_accumulator[1] = 0;

    memset(filter->fir_buf[0], 0, total_fir_len * sizeof(int32_t));

    // Initialise FIR filter stages to the value in filter->signal[REG_MEAS_UNFILTERED]

    while(total_fir_len--)
    {
        regMeasFirFilterRT(filter);
    }

    // Initialise extrapolation buffer

    while(extrapolation_len_iters--)
    {
        *(extrapolation_buf++) = filter->signal[REG_MEAS_FILTERED];
    }

    filter->signal[REG_MEAS_EXTRAPOLATED] = filter->signal[REG_MEAS_FILTERED];

    // Restart the filter

    filter->enable = 1;
}
/*---------------------------------------------------------------------------------------------------------*/
void regMeasSetRegSelect(struct reg_meas_filter *filter, enum reg_meas_select reg_select)
/*---------------------------------------------------------------------------------------------------------*\
  This function sets the selector of the regulation measurement.
\*---------------------------------------------------------------------------------------------------------*/
{
    filter->reg_select = reg_select;
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
//-----------------------------------------------------------------------------------------------------------
// Real-Time Functions
//-----------------------------------------------------------------------------------------------------------
static float regMeasFirFilterRT(struct reg_meas_filter *filter)
/*---------------------------------------------------------------------------------------------------------*\
  This function implements the classical two-stage box-car FIR filter used by regMeasFilter() and
  regMeasFilterInitHistory().
\*---------------------------------------------------------------------------------------------------------*/
{
    int32_t input_integer;
    float   input_meas = filter->signal[REG_MEAS_UNFILTERED];

    // Clip unfiltered input measurement value to avoid crazy roll-overs in the integer stage

    if(input_meas > filter->max_meas_value)
    {
        input_meas = filter->max_meas_value;
    }
    else if(input_meas < -filter->max_meas_value)
    {
        input_meas = -filter->max_meas_value;
    }

    // Filter stage 1

    input_integer = (int32_t)(filter->float_to_integer * input_meas);

    filter->fir_accumulator[0] += (input_integer - filter->fir_buf[0][filter->fir_index[0]]);

    filter->fir_buf[0][filter->fir_index[0]] = input_integer;

    // Do not use modulus (%) operator to wrap fir_index as it is very slow in TMS320C32 DSP

    if(++filter->fir_index[0] >= filter->fir_length[0])
    {
        filter->fir_index[0] = 0;
    }

    // Filter stage 2

    input_integer = filter->fir_accumulator[0] / (int32_t)filter->fir_length[0];

    filter->fir_accumulator[1] += (input_integer - filter->fir_buf[1][filter->fir_index[1]]);

    filter->fir_buf[1][filter->fir_index[1]] = input_integer;

    // Do not use modulus (%) operator to wrap fir_index as it is very slow in TMS320C32 DSP

    if(++filter->fir_index[1] >= filter->fir_length[1])
    {
        filter->fir_index[1] = 0;
    }

    // Convert filter output back to floating point

    return(filter->integer_to_float * (float)filter->fir_accumulator[1]);
}
//-----------------------------------------------------------------------------------------------------------
void regMeasFilterRT(struct reg_meas_filter *filter)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called in real-time to filter the measurement with a 2-stage cascaded box car filter and
  then uses extrapolation to estimate the measurement without the measurement and FIR filtering delay.
  If the filter is not running then the output is simply the unfiltered input.
\*---------------------------------------------------------------------------------------------------------*/
{
    float   old_filtered_value;

    // If filter is stopped

    if(filter->enable == 0)
    {
        // Bypass the filter - simply set the output values to the input value

        filter->signal[REG_MEAS_EXTRAPOLATED] = filter->signal[REG_MEAS_FILTERED] = filter->signal[REG_MEAS_UNFILTERED];
    }
    else // Filter is running
    {
        filter->signal[REG_MEAS_FILTERED] = regMeasFirFilterRT(filter);

        // Prepare to extrapolate to estimate the measurement without a delay

        old_filtered_value = filter->extrapolation_buf[filter->extrapolation_index];

        filter->extrapolation_buf[filter->extrapolation_index] = filter->signal[REG_MEAS_FILTERED];

        // Do not use modulus (%) operator to wrap fir_index as it is very slow in TMS320C32 DSP

        if(++filter->extrapolation_index >= filter->extrapolation_len_iters)
        {
            filter->extrapolation_index = 0;
        }

        // Extrapolate filtered measurement

        filter->signal[REG_MEAS_EXTRAPOLATED] = filter->signal[REG_MEAS_FILTERED] + filter->extrapolation_factor *
                                               (filter->signal[REG_MEAS_FILTERED] - old_filtered_value);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
float regMeasNoiseAndToneRT(struct reg_noise_and_tone *noise_and_tone)
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
void regMeasRateRT(struct reg_meas_rate *meas_rate, float filtered_meas, float period, int32_t period_iters)
/*---------------------------------------------------------------------------------------------------------*\
  This function will store the filtered measurement in the rate estimation history at the regulation
  period (defined by period_iters) and calculate the estimated rate using least-squares regression
  through the past four saved values. See libreg/doc/measurement/least_squares.xlsx for the
  theory behind this implementation.
\*---------------------------------------------------------------------------------------------------------*/
{
    float    *history_buf = meas_rate->history_buf;     // Local pointer to history buffer for efficiency
    uint32_t  idx;                                      // Local copy of index of most recent sample

    // Store measurement at the specified period

    if(++meas_rate->iter_counter >= period_iters)
    {
        meas_rate->iter_counter = 0;
        idx = meas_rate->history_index = (meas_rate->history_index + 1) & REG_MEAS_RATE_BUF_MASK;

        history_buf[idx] = filtered_meas;

        // Estimate rate using linear regression through last four samples

        meas_rate->estimate = (2.0 / 20.0 * period) * (3.0 * (history_buf[ idx ] -
                                                              history_buf[(idx - 3) & REG_MEAS_RATE_BUF_MASK]) +
                                                             (history_buf[(idx - 1) & REG_MEAS_RATE_BUF_MASK]  -
                                                              history_buf[(idx - 2) & REG_MEAS_RATE_BUF_MASK]));
    }
}
// EOF

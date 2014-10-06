/*!
 * @file  meas.c
 * @brief Converter Control Regulation library measurement-related functions
 *
 * <h2>Copyright</h2>
 *
 * Copyright CERN 2014. This project is released under the GNU Lesser General
 * Public License version 3.
 * 
 * <h2>License</h2>
 *
 * This file is part of libreg.
 *
 * libreg is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include "libreg.h"

/*!
 * Classical two-stage box-car FIR filter used by regMeasFilterRT() and regMeasFilterInit().
 *
 * @param[in,out] filter    Measurement filter parameters and values
 * @returns Value of reg_meas_filter::fir_accumulator adjusted by reg_meas_filter::integer_to_float factor
 */
static float regMeasFirFilterRT(struct reg_meas_filter *filter);


// Non-Real-Time Functions - do not call these from the real-time thread or interrupt

void regMeasFilterInitBuffer(struct reg_meas_filter *filter, int32_t *buf)
{
    filter->fir_buf[0] = buf;
}



void regMeasFilterInit(struct reg_meas_filter *filter, uint32_t fir_length[2],
                       uint32_t extrapolation_len_iters, float pos, float neg, float meas_delay_iters)
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
    filter->integer_to_float = 1.0 / (filter->float_to_integer * (float)(filter->fir_length[1]));
    
    // Initialize the FIR filter stages
    
    filter->fir_accumulator[0] = filter->fir_accumulator[1] = 0;

    memset(filter->fir_buf[0], 0, total_fir_len * sizeof(int32_t));

    // Initialize FIR filter stages to the value in filter->signal[REG_MEAS_UNFILTERED]

    while(total_fir_len--)
    {
        filter->signal[REG_MEAS_FILTERED] = regMeasFirFilterRT(filter);
    }

    // Initialize extrapolation buffer

    while(extrapolation_len_iters--)
    {
        *(extrapolation_buf++) = filter->signal[REG_MEAS_FILTERED];
    }

    filter->signal[REG_MEAS_EXTRAPOLATED] = filter->signal[REG_MEAS_FILTERED];

    // Restart the filter

    filter->enable = 1;
}



void regMeasSetNoiseAndTone(struct reg_noise_and_tone *noise_and_tone, float noise_pp,
                            float tone_amp, uint32_t tone_half_period_iters)
{
    noise_and_tone->noise_pp = noise_pp;
    noise_and_tone->tone_amp = tone_amp;
    noise_and_tone->tone_half_period_iters = tone_half_period_iters;
}



// Real-Time Functions

static float regMeasFirFilterRT(struct reg_meas_filter *filter)
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



void regMeasFilterRT(struct reg_meas_filter *filter)
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



float regMeasNoiseAndToneRT(struct reg_noise_and_tone *noise_and_tone)
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



void regMeasRateRT(struct reg_meas_rate *meas_rate, float filtered_meas, float period, int32_t period_iters)
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

        meas_rate->estimate = (2.0 / 20.0) / period * (3.0 * (history_buf[ idx ] -
                                                              history_buf[(idx - 3) & REG_MEAS_RATE_BUF_MASK]) +
                                                             (history_buf[(idx - 1) & REG_MEAS_RATE_BUF_MASK]  -
                                                              history_buf[(idx - 2) & REG_MEAS_RATE_BUF_MASK]));
    }
}

// EOF

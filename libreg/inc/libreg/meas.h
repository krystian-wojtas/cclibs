/*---------------------------------------------------------------------------------------------------------*\
  File:     libreg/meas.h                                                               Copyright CERN 2014

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

  Purpose:  Converter Control Regulation library measurement functions header file

  Contact:  cclibs-devs@cern.ch

  Notes:    Libreg provides a basic 2-stage cascaded box-car filter and then
            extrapolation to compensate for the delay.

\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBREG_MEAS_H
#define LIBREG_MEAS_H

// Include header files

#include <stdint.h>

// Constants

#define REG_MEAS_RATE_BUF_MASK      3                             // Rate will use linear regression through 4 points

// Enum constants

enum reg_meas_select
{
    REG_MEAS_UNFILTERED,
    REG_MEAS_FILTERED,
    REG_MEAS_EXTRAPOLATED,
    REG_MEAS_NUM_SIGNALS
};

// Measurement structures

struct reg_meas_filter                                            // Measurement filter parameters and variables
{
    uint32_t              enable;                                 // Enable filter control

    uint32_t              extrapolation_len_iters;                // Extrapolation length (normally regulation period)
    uint32_t              extrapolation_index;                    // Index to oldest sample in extrapolation buffer

    uint32_t              fir_length[2];                          // FIR filter length for two cascaded stages
    uint32_t              fir_index[2];                           // Index to oldest sample in FIR buffers
    int32_t               fir_accumulator[2];                     // FIR filter accumulator for two cascaded stages

    int32_t              *fir_buf[2];                             // Pointers to buffers for two cascaded FIR stages
    float                *extrapolation_buf;                      // Pointer to buffer for extrapolation stage

    float                 max_meas_value;                         // Maximum value that can be filtered
    float                 float_to_integer;                       // Factor to convert unfiltered measurement to integer
    float                 integer_to_float;                       // Factor to converter integer to filtered measurement
    float                 extrapolation_factor;                   // Extrapolation factor

    enum reg_meas_select  reg_select;                             // Regulation measurement selector
    float                 meas_delay_iters[REG_MEAS_NUM_SIGNALS]; // Delay for each signal in iterations
    float                 meas[REG_MEAS_NUM_SIGNALS];             // Array of measurement with different filtering
};

struct reg_meas_rate                                              // Measurement rate estimate structure
{
    uint32_t              iter_counter;                           // Iteration counter
    uint32_t              buf_index;                              // Index of most recent sample in history buffer
    float                 buf[REG_MEAS_RATE_BUF_MASK+1];          // History buffer
    float                 rate;                                   // Rate of change based on history
};

struct reg_noise_and_tone                                         // Noise and tone generator structure
{
    uint32_t              iter_counter;                           // Iteration counter for simulated tone
    uint32_t              tone_half_period_iters;                 // Tone half period in iterations
    uint32_t              tone_toggle;                            // Tone toggle (0,1,0,1,...)
    float                 tone_amp;                               // Tone amplitude
    float                 noise_pp;                               // Simulated measurement peak-peak noise level
};

#ifdef __cplusplus
extern "C" {
#endif

// Measurement related functions

void     regMeasFilter           (struct reg_meas_filter *filter);
void     regMeasFilterInitBuffer (struct reg_meas_filter *filter, int32_t *buf);
void     regMeasFilterInit       (struct reg_meas_filter *filter, uint32_t fir_length[2],
                                  uint32_t extrapolation_len_iters, float pos, float neg, float meas_delay_iters);
void     regMeasRegSelect        (struct reg_meas_filter *filter, enum reg_meas_select reg_select);
void     regMeasSetNoiseAndTone  (struct reg_noise_and_tone *noise_and_tone, float noise_pp,
                                  float tone_amp, uint32_t tone_half_period_iters);
float    regMeasNoiseAndTone     (struct reg_noise_and_tone *noise_and_tone);
void     regMeasRateStore        (struct reg_meas_rate *meas_rate, float meas, int32_t period_iters);
float    regMeasRate             (struct reg_meas_rate *meas_rate, float period);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_MEAS_H
// EOF


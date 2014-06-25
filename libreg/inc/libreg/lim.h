/*---------------------------------------------------------------------------------------------------------*\
  File:     libreg/lim.h                                                                Copyright CERN 2014

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

  Purpose:  Converter Control Regulation library limits functions header file

  Contact:  cclibs-devs@cern.ch

  Notes:    The limits support includes three types of limits relevant to
            power converter controls:

                1. Field/Current measurement limits (trip)
                2. Field/Current reference limits (clip)
                3. Voltage reference limits (clip)
\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBREG_LIM_H
#define LIBREG_LIM_H

#include <stdint.h>

// Constants

#define REG_LIM_CLIP            0.001                           // Clip limit shift factor
#define REG_LIM_TRIP            0.01                            // Trip limit shift factor
#define REG_LIM_HYSTERESIS      0.1                             // Low/Zero limit hysteresis factor
#define REG_LIM_V_DIODE         -0.6                            // Diode voltage for unipolar flag
#define REG_LIM_FP32_MARGIN     2.0E-07                         // Margin on the relative precision of 32-bit floats

// Limits structures

struct reg_lim_meas                                             // Measurement limits
{
    uint32_t                    invert_limits;                  // Invert the limits before use flag (e.g. polarity switch is negative)
    float                       pos_trip;                       // Positive measurement trip limit
    float                       neg_trip;                       // Negative measurement trip limit
    float                       low;                            // Low measurement threshold
    float                       zero;                           // Zero measurement threshold
    float                       rms2_trip;                      // Squared RMS trip threshold
    float                       rms2_warning;                   // Squared RMS warning threshold
    float                       rms2_warning_hysteresis;        // Squared RMS warning threshold with hysteresis
    float                       meas2_filter;                   // Filtered square of the measurement
    float                       meas2_filter_factor;            // First order filter factor for square of measurement
    float                       low_hysteresis;                 // Low measurement threshold with hysteresis
    float                       zero_hysteresis;                // Zero measurement threshold with hysteresis

    struct                                                      // Measurement limit flags
    {
        uint32_t                trip;                           // Measurement exceeds pos or neg trip limits
        uint32_t                rms_trip;                       // Filtered square of the measurement exceeds trip level
        uint32_t                rms_warning;                    // Filtered square of the measurement exceeds warning level
        uint32_t                low;                            // Absolute measurement is below low threshold
        uint32_t                zero;                           // Absolute measurement is below zero threshold
    } flags;
};

struct reg_lim_ref                                              // Reference limits
{
    uint32_t                    invert_limits;                  // Invert the limits before use flag (e.g. polarity switch is negative)
    float                       max_clip;                       // Max ref clip limit from max_clip_user or Q41 limit
    float                       min_clip;                       // Min ref clip limit from min_clip_user or Q41 limit
    float                       rate_clip;                      // Abs ref rate clip limit

    float                       max_clip_user;                  // Maximum reference clip limit from user
    float                       min_clip_user;                  // Minimum reference clip limit from user

    float                       i_quadrants41_max;              // i_quadrants41[1] or -1.0E10 if no Q41 limits
    float                       v0;                             // Voltage limit for i_meas = 0
    float                       dvdi;                           // Voltage limit slope with i_meas

    struct                                                      // Reference limit flags
    {
        uint32_t                unipolar;                       // Unipolar flag
        uint32_t                clip;                           // Ref is being clipped to [max_clip,min_clip] range
        uint32_t                rate;                           // Ref rate is being clipped to +/-rate_clip range
    } flags;
};

#ifdef __cplusplus
extern "C" {
#endif

// Limits functions

void     regLimMeasInit         (struct reg_lim_meas *lim_meas, float pos_lim, float neg_lim,
                                 float low_lim, float zero_lim, uint32_t invert_limits);
void     regLimMeasRmsInit      (struct reg_lim_meas *lim_meas, float rms_trip, float rms_warning, float rms_tc, float iter_period);
void     regLimMeasRT           (struct reg_lim_meas *lim_meas, float meas);
void     regLimRefInit          (struct reg_lim_ref *lim_ref, float pos_lim, float neg_lim, float rate_lim,
                                 uint32_t invert_limits);
void     regLimVrefInit         (struct reg_lim_ref *lim_v_ref, float pos_lim, float neg_lim, float rate_lim,
                                 float i_quadrants41[2], float v_quadrants41[2], uint32_t invert_limits);
void     regLimVrefCalcRT       (struct reg_lim_ref *lim_v_ref, float i_meas);
float    regLimRefRT            (struct reg_lim_ref *lim_ref, float period, float ref, float prev_ref);

#ifdef __cplusplus
}
#endif

// inline function definitions

static inline void regLimRefSetInvertLimits(struct reg_lim_ref *lim_ref, uint32_t invert_limits)
{
    lim_ref->invert_limits = invert_limits;
}

#endif // LIBREG_LIM_H

// EOF

/*!
 * @file  fgTest.c
 * @brief Generate test functions (STEPS, SQUARE, SINE or COSINE)
 *
 * <h2>Copyright</h2>
 *
 * Copyright CERN 2015. This project is released under the GNU Lesser General
 * Public License version 3.
 * 
 * <h2>License</h2>
 *
 * This file is part of libfg.
 *
 * libfg is free software: you can redistribute it and/or modify it under the
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
#include "libfg/test.h"



enum fg_error fgTestInit(struct fg_limits *limits, 
                         bool   is_pol_switch_auto,
                         bool   is_pol_switch_neg,
                         double delay, 
                         enum   fg_test_type type,
                         float  initial_ref,
                         float  amplitude_pp,
                         float  num_cycles,
                         float  period,
                         bool   is_window_active,
                         struct fg_test *pars, 
                         struct fg_meta *meta)
{
    enum fg_error  fg_error;       // Error code
    struct fg_meta local_meta;     // Local meta data in case user meta is NULL
    struct fg_test p;              // Local TEST pars - copied to user *pars only if there are no errors
    float          window[2];      // Max/min window scaling for sine or cosine

    // Reset meta structure - uses local_meta if meta is NULL

    meta = fgResetMeta(meta, &local_meta, delay, initial_ref);

    // Check if number of cycles is less than 1

    if(num_cycles < 0.6)
    {
        meta->error.index   = 1;
        meta->error.data[0] = num_cycles;

        fg_error = FG_INVALID_TIME;
        goto error;
    }

    // Prepare parameter structure

    p.delay            = delay;
    p.num_cycles       = (uint32_t)(num_cycles + 0.4999);
    p.duration         = (float)p.num_cycles * period;
    p.half_period      = 0.5 * period;
    p.frequency        = 1.0 / period;
    p.amplitude        = amplitude_pp;
    p.type             = type;
    p.is_window_active = is_window_active;
    p.initial_ref      = initial_ref;
    p.final_ref        = initial_ref;

    // Check if total duration is too long

    if(p.duration > 1.0E6)
    {
        meta->error.index   = 2;
        meta->error.data[0] = p.duration;
        meta->error.data[1] = 1.0E6;

        fg_error = FG_INVALID_TIME;
        goto error;
    }

    // Prepare range scaling if window is active and the number of cycles is 1

    if(p.is_window_active && p.num_cycles == 1)
    {
        if(type == FG_TEST_SINE)            // Windowed SINE
        {
            window[0] =  0.649519053;            // +3.sqrt(3)/8
            window[1] = -0.649519053;            // -3.sqrt(3)/8
        }
        else                                // Windowed COSINE
        {
            window[0] =  1.0 / 8.0;              // +1/8
            window[1] = -1.0;                    // -1
        }
    }
    else    // Window not active the whole time
    {
        window[0] =  1.0;
        window[1] = -1.0;
    }

    // Calculate amplitude related parameters

    fg_error = FG_OK;

    switch(type)
    {
        case FG_TEST_STEPS:

            p.final_ref += p.amplitude;
            p.amplitude   /= (float)p.num_cycles;

            fgSetMinMax(meta, p.final_ref);
            break;

        case FG_TEST_SQUARE:
 
            // Square wave is created from 2 x half cycles

            p.num_cycles *= 2;

            fgSetMinMax(meta, p.initial_ref + p.amplitude);
            break;

        case FG_TEST_SINE:
        case FG_TEST_COSINE:

            p.amplitude *= 0.5;

            fgSetMinMax(meta, initial_ref + p.amplitude * window[0]);
            fgSetMinMax(meta, initial_ref + p.amplitude * window[1]);
            break;

        default: // Invalid function type requested

            meta->error.data[0] = (float)type;

            fg_error = FG_BAD_PARAMETER;
            goto error;
    }

    // Complete meta data

    meta->duration  = p.duration;
    meta->range.end = p.final_ref;;

    fgSetFuncPolarity(meta, is_pol_switch_auto, is_pol_switch_neg);

    // Copy valid set of parameters to user's pars structure

    memcpy(pars, &p, sizeof(p));

    return(FG_OK);

    // Error - store error code in meta and return to caller

    error:

        meta->fg_error = fg_error;
        return(fg_error);
}



enum fg_gen_status fgTestGen(struct fg_test *pars, const double *time, float *ref)
{
    double      radians;
    float       cos_rads = 0.0;
    float       delta_ref;
    double      func_time;                     // Time within function

    // Both *time and delay must be 64-bit doubles if time is UNIX time

    func_time = *time - pars->delay;

    // Pre-acceleration coast

    if(func_time < 0.0)
    {
        *ref = pars->initial_ref;

        return(FG_GEN_BEFORE_FUNC);
    }

    // Operate N cycles following delay

    else if(func_time < pars->duration)
    {
        uint32_t    period_idx;

        switch(pars->type)
        {
            case FG_TEST_STEPS:

                // Calculate period index and clip to number of cycles in case of floating point errors

                period_idx = 1 + (uint32_t)(func_time * pars->frequency);

                if(period_idx > pars->num_cycles)
                {
                    period_idx = pars->num_cycles;
                }

                *ref = pars->initial_ref + pars->amplitude * (float)period_idx;

                return(FG_GEN_DURING_FUNC);

            case FG_TEST_SQUARE:

                // Calculate period index and clip to number of cycles in case of floating point errors

                period_idx = 1 + (uint32_t)(2.0 * func_time * pars->frequency);

                if(period_idx > pars->num_cycles)
                {
                    period_idx = pars->num_cycles;
                }

                *ref = pars->initial_ref + (period_idx & 0x1 ? pars->amplitude : 0.0);

                return(FG_GEN_DURING_FUNC);

            case FG_TEST_SINE:

                radians   = (2.0 * 3.1415926535897932) * pars->frequency * func_time;
                delta_ref = pars->amplitude * sin(radians);
                break;

            case FG_TEST_COSINE:

                radians   = (2.0 * 3.1415926535897932) * pars->frequency * func_time;
                cos_rads  = cos(radians);
                delta_ref = pars->amplitude * cos_rads;
                break;

            default: // Invalid function type requested

                return(FG_GEN_AFTER_FUNC);
        }

        // For SINE and COSINE: Apply cosine window if enabled

        if(pars->is_window_active &&
          (func_time < pars->half_period || pars->duration - func_time < pars->half_period))
        {
            // Calc Cosine window

           delta_ref *= 0.5 * (1 - (pars->type == FG_TEST_SINE ? cos(radians) : cos_rads));
        }

        *ref = pars->initial_ref + delta_ref;

        return(FG_GEN_DURING_FUNC);
    }

    // Coast after function

    *ref = pars->final_ref;

    return(FG_GEN_AFTER_FUNC);
}

// EOF

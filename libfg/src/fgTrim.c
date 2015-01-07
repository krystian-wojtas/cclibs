/*!
 * @file  fgTrim.c
 * @brief Generate linear and cubic trim functions
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
#include "libfg/trim.h"



enum fg_error fgTrimInit(struct fg_limits *limits, 
                         bool   is_pol_switch_auto,
                         bool   is_pol_switch_neg,
                         double delay, 
                         enum   fg_trim_type type,
                         float  initial_ref,
                         float  final_ref,
                         float  duration,
                         struct fg_trim *pars, 
                         struct fg_meta *meta)
{
    enum fg_error  fg_error;         // Error code
    struct fg_meta local_meta;       // Local meta data in case user meta is NULL
    struct fg_trim p;                // Local TRIM pars - copied to user *pars only if there are no errors
    bool           is_trim_inverted; // Inverted trim flag
    float          acceleration;     // Acceleration
    float          duration1;        // Trim duration
    float          duration2;        // Trim duration based on acceleration limit
    float          delta_ref;        // Trim reference change
    float          rate_lim;         // Limiting

    // Reset meta structure - uses local_meta if meta is NULL

    meta = fgResetMeta(meta, &local_meta, delay, initial_ref);

    // Save parameters

    p.delay       = delay;
    p.initial_ref = initial_ref;
    p.final_ref   = final_ref;

    // Assess if ramp is rising or falling

    delta_ref = p.final_ref - initial_ref;

    if(delta_ref < 0.0)
    {
        is_trim_inverted = true;
        meta->range.min  = p.final_ref;
        meta->range.max  = p.initial_ref;
    }
    else
    {
        is_trim_inverted = false;
        meta->range.min  = p.initial_ref;
        meta->range.max  = p.final_ref;
    }

    // Prepare cubic factors according to trim type

    switch(type)
    {
        case FG_TRIM_CUBIC:

            p.a = 1.0;
            p.c = 1.5;
            break;

        case FG_TRIM_LINEAR:

            p.a = 0.0;
            p.c = 1.0;
            break;

        default:

            meta->error.index   = 1;
            meta->error.data[0] = type;

            fg_error = FG_BAD_PARAMETER;
            goto error;
    }

    // Calculate or check duration and complete cubic factors

    if(duration < 1.0E-6)                
    {
        // If duration is zero then limits must be supplied

        if(limits == NULL || limits->rate == 0.0)              
        {
            meta->error.index = 2;

            fg_error = FG_BAD_PARAMETER;
            goto error;
        }

        // Calculate rate of change limit

        rate_lim = limits->rate;

        if(is_trim_inverted)
        {
            rate_lim = -rate_lim;
        }

        // Calculate duration based on rate_lim

        duration1 = p.c * delta_ref / rate_lim;

        // If Cubic trim

        if(p.a != 0.0)
        {
            // Protect against zero acceleration

            if(limits->acceleration <= 1.0E-6)
            {
                meta->error.index   = 3;
                meta->error.data[0] = limits->acceleration;

                fg_error = FG_BAD_PARAMETER;
                goto error;
            }

            // Calculate duration based on acceleration limit

            duration2 = sqrt(fabs(6.0 * delta_ref / limits->acceleration));     
                                                                               
            if(duration1 < duration2)                                          
            {
                duration1 = duration2;
            }
        }

        duration = duration1;
    }

    p.a *= -2.0 * delta_ref / (duration * duration * duration);
    p.c *= delta_ref / duration;

    // Calculate offsets

    p.time_offset = 0.5 * duration;
    p.ref_offset  = 0.5 * (p.initial_ref + p.final_ref);

    // Calculate acceleration  (note a=0 for Linear trim)

    acceleration = fabs(3.0 * p.a * duration);

    // Complete meta data

    meta->duration  = p.duration = duration;
    meta->range.end = p.final_ref;

    fgSetFuncPolarity(meta, is_pol_switch_auto, is_pol_switch_neg);

    // Check limits at the beginning, middle and end if supplied

    if(limits != NULL)
    {
        if((fg_error = fgCheckRef(limits, p.initial_ref, 0.0, type == FG_TRIM_CUBIC ? acceleration : 0.0, meta)) ||
           (fg_error = fgCheckRef(limits, p.ref_offset,  p.c, 0.0, meta)) ||
           (fg_error = fgCheckRef(limits, p.final_ref,   0.0, 0.0, meta)))
        {
            goto error;
        }
    }

    // Copy valid set of parameters to user's pars structure

    memcpy(pars, &p, sizeof(p));

    return(FG_OK);

    // Error - store error code in meta and return to caller

    error:

        meta->fg_error = fg_error;
        return(fg_error);
}



enum fg_gen_status fgTrimGen(struct fg_trim *pars, const double *time, float *ref)
{
    double   func_time;                     // Time within function
    float    seg_time;                      // Time within segment

    // Both *time and delay must be 64-bit doubles if time is UNIX time

    func_time = *time - pars->delay;

    // Pre-trim coast

    if(func_time < 0.0)
    {
        *ref = pars->initial_ref;

        return(FG_GEN_BEFORE_FUNC);
    }

    // Trim

    else if(func_time <= pars->duration)
    {
        seg_time = func_time - pars->time_offset;
        *ref   = pars->ref_offset + seg_time * (pars->a * seg_time * seg_time + pars->c);

        return(FG_GEN_DURING_FUNC);
    }

    // Post-trim coast

    *ref = pars->final_ref;

    return(FG_GEN_AFTER_FUNC);
}

// EOF

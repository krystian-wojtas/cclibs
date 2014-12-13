/*!
 * @file  fgTrim.c
 * @brief Generate linear and cubic trim functions
 *
 * <h2>Copyright</h2>
 *
 * Copyright CERN 2014. This project is released under the GNU Lesser General
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



enum fg_error fgTrimInit(struct fg_limits          *limits,
                         enum   fg_limits_polarity  limits_polarity,
                         struct fg_trim_config     *config,
                         float                      delay,
                         float                      init_ref,
                         struct fg_trim_pars       *pars,
                         struct fg_meta            *meta)
{
    enum fg_error  fg_error;         // Error code
    struct fg_meta local_meta;       // Local meta data in case user meta is NULL
    struct fg_trim_pars p;           // Local TRIM pars - copied to user *pars only if there are no errors
    bool           is_trim_inverted; // Inverted trim flag
    float          acceleration;     // Acceleration
    float          duration;         // Trim duration
    float          duration2;        // Trim duration based on acceleration limit
    float          delta_ref;        // Trim reference change
    float          rate_lim;         // Limiting

    // Reset meta structure - uses local_meta if meta is NULL

    meta = fgResetMeta(meta, &local_meta, init_ref);

    // Save parameters

    p.delay       = delay;
    p.ref_initial = init_ref;
    p.ref_final   = config->final;

    // Assess if ramp is rising or falling

    delta_ref = p.ref_final - init_ref;

    if(delta_ref < 0.0)
    {
        is_trim_inverted = true;
        meta->range.min  = p.ref_final;
        meta->range.max  = p.ref_initial;
    }
    else
    {
        is_trim_inverted = false;
        meta->range.min  = p.ref_initial;
        meta->range.max  = p.ref_final;
    }

    // Prepare cubic factors according to trim type

    switch(config->type)
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

            if(meta != NULL)
            {
                meta->error.index   = 1;
                meta->error.data[0] = config->type;
            }

            fg_error = FG_BAD_PARAMETER;
            goto error;
    }

    // Calculate or check duration and complete cubic factors

    if(config->duration < 1.0E-6)                                 // If duration is zero
    {
        if(limits == NULL)                                              // Check that limits were supplied
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

        duration = p.c * delta_ref / rate_lim;                      // Calculate duration based on rate_lim

        if(p.a != 0.0)                                                  // If Cubic trim
        {
            if(limits->acceleration <= 0.0)                                     // Protect against zero acc
            {
                meta->error.index   = 3;
                meta->error.data[0] = limits->acceleration;

                fg_error = FG_BAD_PARAMETER;
                goto error;
            }

            duration2 = sqrt(fabs(6.0 * delta_ref / limits->acceleration));     // Calc duration based on
                                                                                // acceleration limit
            if(duration < duration2)                                            // and use the longer duration
            {
                duration = duration2;
            }
        }

        config->duration = duration;                                    // Set duration to calculated value
    }
    else                                                        // else user has supplied a duration
    {
        duration = config->duration;
    }

    p.a *= -2.0 * delta_ref / (duration * duration * duration);
    p.c *= delta_ref / duration;

    // Calculate offsets

    p.time_offset = 0.5 * duration;
    p.ref_offset  = 0.5 * (p.ref_initial + p.ref_final);

    // Calculate acceleration  (note a=0 for Linear trim)

    acceleration = fabs(3.0 * p.a * duration);

    // If supplied, check limits at the beginning, middle and end

    if(limits != NULL)
    {
        if((fg_error = fgCheckRef(limits, limits_polarity, p.ref_initial, 0.0, (config->type == FG_TRIM_CUBIC ? acceleration : 0.0), meta)) ||
           (fg_error = fgCheckRef(limits, limits_polarity, p.ref_offset,  p.c, 0.0, meta)) ||
           (fg_error = fgCheckRef(limits, limits_polarity, p.ref_final,   0.0, 0.0, meta)))
        {
            goto error;
        }
    }

    // Complete meta data

    meta->duration  = p.duration = duration;
    meta->range.end = p.ref_final;

    // Copy valid set of parameters to user's pars structure

    memcpy(pars, &p, sizeof(p));

    return(FG_OK);

    // Error - store error code in meta and return to caller

    error:

        meta->fg_error = fg_error;
        return(fg_error);
}



bool fgTrimGen(struct fg_trim_pars *pars, const double *time, float *ref)
{
    double   func_time;                     // Time within function
    float    seg_time;                      // Time within segment

    // Both *time and delay must be 64-bit doubles if time is UNIX time

    func_time = *time - pars->delay;

    // Pre-trim coast

    if(func_time <= 0.0)
    {
        *ref = pars->ref_initial;
    }

    // Trim

    else if(func_time <= pars->duration)
    {
        seg_time = func_time - pars->time_offset;
        *ref   = pars->ref_offset + seg_time * (pars->a * seg_time * seg_time + pars->c);
    }

    // Post-trim coast

    else
    {
        *ref = pars->ref_final;

        return(false);
    }

    return(true);
}

// EOF

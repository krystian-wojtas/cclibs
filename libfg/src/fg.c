/*!
 * @file  fg.c
 * @brief Function Generation library top-level source file.
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

#include "libfg.h"



struct fg_meta * fgResetMeta(struct fg_meta *meta, struct fg_meta *local_meta, double delay, float initial_ref)
{
    uint32_t idx;

    // If user supplied meta pointer is NULL then use local_meta from libreg Init function

    if(meta == NULL)
    {
        meta = local_meta;
    }

    // Reset all field in meta structure if pointer is valid

    if(meta != NULL)
    {
        // Explicitly set floats to zero because on early TI DSPs do not use IEEE758 
        // floating point format. For example, for the TMS320C32, 0.0 is 0x80000000 
        // while 0x00000000 has the value 1.0.

        for(idx = 0 ; idx < FG_ERR_DATA_LEN ; idx++)
        {
            meta->error.data[idx] = 0.0;
        }

        meta->error.index     = 0;
        meta->polarity        = FG_FUNC_POL_ZERO;
        meta->limits_inverted = false;
        meta->delay           = delay;
        meta->duration        = 0.0;
        meta->range.end       = 0.0;
        meta->range.start     = initial_ref;
        meta->range.min       = initial_ref;
        meta->range.max       = initial_ref;
    }

    return(meta);
}



void fgSetMinMax(struct fg_meta *meta, float ref)
{
    if(ref > meta->range.max)
    {
        meta->range.max = ref;

    }
    else if(ref < meta->range.min)
    {
        meta->range.min = ref;
    }
}



void fgSetFuncPolarity(struct fg_meta *meta,
                       bool   is_pol_switch_auto,
                       bool   is_pol_switch_neg)
{
    if(meta->range.max > 0.0)
    {
        meta->polarity |= FG_FUNC_POL_POSITIVE;
    }

    if(meta->range.min < 0.0)
    {
        meta->polarity |= FG_FUNC_POL_NEGATIVE;
    }

    // Set limits inversion control based on the switch control and state

    meta->limits_inverted =(is_pol_switch_auto == false && is_pol_switch_neg == true) || 
                           (is_pol_switch_auto == true  && meta->polarity == FG_FUNC_POL_NEGATIVE);
}



enum fg_error fgCheckRef(struct fg_limits *limits, 
                         float  ref, 
                         float  rate, 
                         float  acceleration, 
                         struct fg_meta *meta)
{
    float    max;
    float    min;
    float    limit;

    // Do nothing if limits are NULL

    if(limits == NULL)
    {
        return(FG_OK);
    }

    // Invert limits if necessary

    if(meta->limits_inverted)
    {
        // Invert limits - only required for unipolar converters so limits->neg will be zero

        max = -(1.0 - FG_CLIP_LIMIT_FACTOR) * limits->min;
        min = -(1.0 + FG_CLIP_LIMIT_FACTOR) * limits->pos;
    }
    else // Limits do not need to be inverted
    {
        max = (1.0 + FG_CLIP_LIMIT_FACTOR) * limits->pos;
        min = (limits->neg < 0.0 ? (1.0 + FG_CLIP_LIMIT_FACTOR) * limits->neg :
                                   (1.0 - FG_CLIP_LIMIT_FACTOR) * limits->min);
    }

    // Check reference level

    if(ref > max || ref < min)
    {
        meta->error.data[0] = ref;
        meta->error.data[1] = min;
        meta->error.data[2] = max;

        return(FG_OUT_OF_LIMITS);
    }

    // Check rate of change if limit is positive

    if(limits->rate >  0.0 &&
       fabs(rate)   > (limit =((1.0 + FG_CLIP_LIMIT_FACTOR) * limits->rate)))
    {
        meta->error.data[0] = rate;
        meta->error.data[1] = limit;
        meta->error.data[2] = limits->rate;

        return(FG_OUT_OF_RATE_LIMITS);
    }

    // Check acceleration

    if(limits->acceleration >  0.0 &&
       fabs(acceleration)   > (limit = ((1.0 + FG_CLIP_LIMIT_FACTOR) * limits->acceleration)))
    {
        meta->error.data[0] = acceleration;
        meta->error.data[1] = limit;
        meta->error.data[2] = limits->acceleration;

        return(FG_OUT_OF_ACCELERATION_LIMITS);
    }

    return(FG_OK);
}

// EOF

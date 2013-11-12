/*---------------------------------------------------------------------------------------------------------*\
  File:     fg.c                                                                        Copyright CERN 2011

  License:  This file is part of libfg.

            libfg is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  Function generation library top level source file
\*---------------------------------------------------------------------------------------------------------*/

#include "libfg.h"

/*---------------------------------------------------------------------------------------------------------*/
void fgResetMeta(struct fg_meta *meta)
/*---------------------------------------------------------------------------------------------------------*\
  When a function is initialised a meta structure is filled with a summary of the function including the
  min/max and start and end values.  This function is used to reset all the fields in the meta structure.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t idx;

    if(meta != NULL)
    {
        for(idx = 0 ; idx < FG_ERR_DATA_LEN ; idx++)
        {
            meta->error.data[idx] = 0.0;
        }
        meta->error.index = 0;
        meta->duration    = 0.0;
        meta->range.start = 0.0;
        meta->range.end   = 0.0;
        meta->range.min   = 0.0;
        meta->range.max   = 0.0;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error fgCheckRef(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                         uint32_t negative_flag, float ref, float rate, float acceleration,
                         struct fg_meta *meta)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called when a function is initialised to check the function value, rate and acceleration
  against the supplied limits.  It will also call a user supplied call back if supplied to allow further
  checks.
\*---------------------------------------------------------------------------------------------------------*/
{
    float   pos;
    float   neg;

    // Do nothing if limits are NULL

    if(!limits)
    {
        return(FG_OK);
    }

    // Invert limits if necessary

    if( limits_polarity == FG_LIMITS_POL_NEGATIVE ||
       (limits_polarity == FG_LIMITS_POL_AUTO && negative_flag))
    {
        // Invert limits - only required for unipolar converters so limits->neg will be zero

        pos = -(1.0 - FG_CLIP_LIMIT_FACTOR) * limits->min;
        neg = -(1.0 + FG_CLIP_LIMIT_FACTOR) * limits->pos;
    }
    else // Limits do not need to be inverted
    {
        pos = (1.0 + FG_CLIP_LIMIT_FACTOR) * limits->pos;
        neg = (limits->neg < 0.0 ? (1.0 + FG_CLIP_LIMIT_FACTOR) * limits->neg :
                                   (1.0 - FG_CLIP_LIMIT_FACTOR) * limits->min);
    }

    // Check reference level

    if(ref > pos || ref < neg)
    {
        if(meta != NULL)
        {
            meta->error.data[0] = neg;
            meta->error.data[1] = ref;
            meta->error.data[2] = pos;
        }

        return(FG_OUT_OF_LIMITS);
    }

    // Check rate of change

    if(limits->rate >  0.0 &&
       fabs(rate)   > (1.0 + FG_CLIP_LIMIT_FACTOR) * limits->rate)
    {
        if(meta != NULL)
        {
            meta->error.data[0] = limits->rate;
            meta->error.data[1] = (1.0 + FG_CLIP_LIMIT_FACTOR) * limits->rate;
            meta->error.data[2] = rate;
        }

        return(FG_OUT_OF_RATE_LIMITS);
    }

    // Check acceleration

    if(limits->acceleration >  0.0 &&
       fabs(acceleration)   > (1.0 + FG_CLIP_LIMIT_FACTOR) * limits->acceleration)
    {
        if(meta != NULL)
        {
            meta->error.data[0] = limits->acceleration;
            meta->error.data[1] = (1.0 + FG_CLIP_LIMIT_FACTOR) * limits->acceleration;
            meta->error.data[2] = acceleration;
        }

        return(FG_OUT_OF_ACCELERATION_LIMITS);
    }

    // Call user function to check reference if supplied

    if(limits->user_check_limits)
    {
        return(limits->user_check_limits(limits, limits_polarity, negative_flag, ref, rate, acceleration));
    }

    return(FG_OK);
}
/*---------------------------------------------------------------------------------------------------------*\
  End of file: fg.c
\*---------------------------------------------------------------------------------------------------------*/

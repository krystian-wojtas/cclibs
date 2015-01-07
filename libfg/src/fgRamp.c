/*!
 * @file  fgRamp.c
 * @brief Generate fast ramp based on Parabola-Parabola function
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
#include "libfg/ramp.h"



enum fg_error fgRampInit(struct fg_limits *limits, 
                         bool   is_pol_switch_auto,
                         bool   is_pol_switch_neg,
                         double delay, 
                         float  initial_ref,
                         float  final_ref,
                         float  acceleration,
                         float  linear_rate,
                         float  deceleration,
                         struct fg_ramp *pars, 
                         struct fg_meta *meta)
{
    enum fg_error  fg_error;                     // Reference limits status
    struct fg_meta local_meta;                   // Local meta data in case user meta is NULL
    struct fg_ramp p;                            // Local RAMP pars - copied to user *pars if there are no errors

    // Check that parameters are valid

    if(acceleration == 0.0 || deceleration == 0.0)
    {
        // Reset meta structure - uses local_meta if meta is NULL

        meta = fgResetMeta(meta, &local_meta, delay, initial_ref);

        fg_error = FG_BAD_PARAMETER;
        goto error;
    }

    // Calculate ramp parameters always with zero initial ramp rate

    fgRampCalc(is_pol_switch_auto, is_pol_switch_neg, 
               delay, 0.0, initial_ref, final_ref, acceleration, linear_rate, deceleration, &p, meta);
                         

    // Check limits if supplied

    if(limits != NULL)
    {
        // Check limits at the start of the parabolic acceleration (segment 1)

        if((fg_error = fgCheckRef(limits, initial_ref, 0.0, p.acceleration, meta)))
        {
            meta->error.index = 1;
            goto error;
        }

        // Check limits at the end of the parabolic deceleration (segment 2)

        if((fg_error = fgCheckRef(limits, final_ref, 0.0, p.deceleration, meta)))
        {
            meta->error.index = 2;
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



void fgRampCalc(bool   is_pol_switch_auto,
                bool   is_pol_switch_neg,
                double delay, 
                float  initial_rate,
                float  initial_ref,
                float  final_ref,
                float  acceleration,
                float  linear_rate,
                float  deceleration,
                struct fg_ramp *pars, 
                struct fg_meta *meta)
{
    float           delta_ref;              // Initial ref minus final ref
    float           ref0;                   // Ref at t=0 for first parabola
    float           overshoot_rate_limit;   // Limiting initial rate of change before overshoot occurs
    float           seg_ratio;              // Ratio between the two segments
    struct fg_meta  local_meta;             // Local meta data in case user meta is NULL

    // Prepare meta structure

    meta = fgResetMeta(meta, &local_meta, delay, initial_ref);

    // Prepare variables assuming ascending (positive) ramp

    pars->delay             =  delay;
    pars->prev_time         =  delay;
    pars->is_ramp_positive  =  true;
    pars->acceleration      =  fabs(acceleration);
    pars->deceleration      = -fabs(deceleration);
    pars->linear_rate       =  fabs(linear_rate);
    pars->linear_rate_limit =  fabs(initial_rate);
    pars->prev_ramp_ref     =  pars->prev_returned_ref = pars->initial_ref = initial_ref;
    delta_ref               =  final_ref - initial_ref;
    overshoot_rate_limit    =  sqrt(-2.0 * pars->deceleration * fabs(delta_ref));

    // Set up accelerations according to ramp direction and possible overshoot

    if((delta_ref >= 0.0 && initial_rate >   overshoot_rate_limit) ||
       (delta_ref <  0.0 && initial_rate >= -overshoot_rate_limit))
    {
        // Descending (negative) ramp
        
            pars->is_ramp_positive =  false;
            pars->acceleration  = -pars->acceleration;
            pars->deceleration  = -pars->deceleration;
    }

    // Set time_shift and ref0 and delta_ref to take into account the initial rate of change

    pars->time_shift  = -initial_rate / pars->acceleration;
    pars->is_pre_ramp = (pars->time_shift > 0.0);

    ref0      = initial_ref + 0.5 * initial_rate * pars->time_shift;
    delta_ref = final_ref - ref0;

    // Calculate ramp parameters

    seg_ratio = pars->deceleration / (pars->deceleration - pars->acceleration);

    pars->time[0] = 0.0;
    pars->time[2] = sqrt(2.0 * delta_ref / (seg_ratio * pars->acceleration));
    pars->time[1] = pars->time[2] * seg_ratio;

    pars->ref[0]  = ref0;
    pars->ref[1]  = ref0 + delta_ref * seg_ratio;
    pars->ref[2]  = final_ref;

    // Set duration if rate limit is never reached

    meta->duration = pars->time[2] + pars->time_shift;

    // Set min/max

    fgSetMinMax(meta,initial_ref);
    fgSetMinMax(meta,final_ref);

    // If time_shift is positive then include point of inflexion of first parabola in min/max check

    if(pars->time_shift > 0.0)
    {
        fgSetMinMax(meta,ref0);
    }

    // Complete meta data

    meta->range.end = final_ref;

    fgSetFuncPolarity(meta, is_pol_switch_auto, is_pol_switch_neg);
}



enum fg_gen_status fgRampGen(struct fg_ramp *pars, const double *time, float *ref)
{
    enum fg_gen_status status = FG_GEN_DURING_FUNC; // Set default return status
    uint32_t    time_shift_alg  = 0;                // Time shift adjustment algorithm index
    float       r;
    float       ref_rate_limit;                     // Limit on ref due to rate limit
    float       period;                             // Time period calculated using prev_time
    double      ref_time;                           // Time within the segment in seconds

    // Pre-function coast

    if(*time < pars->delay)
    {
        r = pars->initial_ref;

        status = FG_GEN_BEFORE_FUNC;
    }
    else
    {
        // If reference received from previous iteration was changed, and isn't blocked

        if(*ref != pars->prev_ramp_ref && *ref != pars->prev_returned_ref)
        {
            // Identify time shift adjustment algorithm according to ramp direction

            if(pars->is_ramp_positive)
            {
                // Positive (rising) ramp 

                if(*ref > pars->ref[0])
                {
                    if(pars->is_pre_ramp)
                    {
                         time_shift_alg = 1;
                    }
                    else if(*ref <= pars->ref[1])
                    {
                         time_shift_alg = 2;
                    }
                    else if(*ref <= pars->ref[2])
                    {
                         time_shift_alg = 3;
                    }
                }
            }
            else // Negative (falling) ramp
            {
                if(*ref < pars->ref[0])
                {
                    if(pars->is_pre_ramp)
                    {
                         time_shift_alg = 1;
                    }
                    else if(*ref >= pars->ref[1])
                    {
                         time_shift_alg = 2;
                    }
                    else if(*ref >= pars->ref[2])
                    {
                         time_shift_alg = 3;
                    }
                }
            }

            // Adjust time shift using appropriate algorithm

            switch(time_shift_alg)
            {
                case 1: pars->time_shift = pars->prev_time - pars->delay +
                                           sqrt(2.0 * (*ref - pars->ref[0]) / pars->acceleration);
                        break;

                case 2: pars->time_shift = pars->prev_time - pars->delay -
                                           sqrt(2.0 * (*ref - pars->ref[0]) / pars->acceleration);
                        break;

                case 3: pars->time_shift = pars->prev_time - pars->delay -
                                           (pars->time[2] - sqrt(2.0 * (*ref - pars->ref[2]) / pars->deceleration)); // deceleration always +ve
                        break;

                default:break;
            }
        }

        // Calculate new ref_time including delay and time_shift

        ref_time = *time - pars->delay - pars->time_shift;

        // Parabolic acceleration

        if(ref_time <= pars->time[1])
        {
            r = pars->ref[0] + 0.5 * pars->acceleration * ref_time * ref_time;

            // Reset is_pre_ramp once the main part of the ramp is started

            if(ref_time >= 0.0)
            {
                pars->is_pre_ramp = false;
            }
        }

        // Parabolic deceleration

        else if(ref_time <= pars->time[2])
        {
            ref_time -= pars->time[2];        // ref_time is relative to end of parabola (negative)
            r = pars->ref[2] + 0.5 * pars->deceleration * ref_time * ref_time;
        }

        // Coast

        else
        {
            r = pars->ref[2];

            // End of function
 
            status = FG_GEN_AFTER_FUNC;
        }

        // Keep ramp reference for next iteration (before rate limiter)

        pars->prev_ramp_ref = r;

        // Apply rate limit if active

        period = *time - pars->prev_time;

        if(pars->linear_rate > 0.0 && period > 0.0)
        {
            if(pars->linear_rate_limit < pars->linear_rate)
            {
                pars->linear_rate_limit = pars->linear_rate;
            }

            if(r > *ref)
            {
                // Positive rate of change

                ref_rate_limit = *ref + pars->linear_rate_limit * period;

                if(r > ref_rate_limit)
                {
                    r = ref_rate_limit;
                }
            }
            else if(r < *ref)
            {
                // Negative rate of change

                ref_rate_limit = *ref - pars->linear_rate_limit * period;

                if(r < ref_rate_limit)
                {
                    r = ref_rate_limit;
                }
            }

            // Adjust linear rate limit if greater than user value, respecting acceleration

            if(pars->linear_rate_limit > pars->linear_rate)
            {
                pars->linear_rate_limit -= period * fabs(ref_time <= pars->time[1] ? pars->acceleration : pars->deceleration);
            }
        }
    }

    // Keep returned reference and time for next iteration

    pars->prev_returned_ref = *ref;
    pars->prev_time         = *time;

    // Return new reference after rate limit

    *ref = r;

    return(status);
}

// EOF

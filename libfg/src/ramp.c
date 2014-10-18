/*!
 * @file  ramp.c
 * @brief Generate fast ramp based on Parabola-Parabola function
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

#include "libfg/ramp.h"

enum fg_error fgRampInit(struct fg_limits          *limits,
                         enum   fg_limits_polarity  limits_polarity,
                         struct fg_ramp_config     *config,
                         double                     delay,
                         float                      init_ref,
                         struct fg_ramp_pars       *pars,
                         struct fg_meta            *meta)
{
    enum fg_error  fg_error;                     // Reference limits status
    struct fg_meta local_meta;                   // Local meta data in case user meta is NULL


    // Check that parameters are valid

    if(config->acceleration == 0.0 || config->deceleration == 0.0)
    {
        // Reset meta structure if provided

        meta = fgResetMeta(meta, &local_meta, init_ref);

        return(FG_BAD_PARAMETER);
    }

    // Calculate ramp parameters always with zero initial ramp rate

    fgRampCalc(config, delay, init_ref, 0.0, pars, meta);

    // Check limits if supplied

    if(limits != NULL)
    {
        // Check limits at the start of the parabolic acceleration (segment 1)

        if((fg_error = fgCheckRef(limits, limits_polarity, init_ref, 0.0, pars->acceleration, meta)))
        {
            meta->error.index = 1;
            return(fg_error);
        }

        // Check limits at the end of the parabolic deceleration (segment 2)

        if((fg_error = fgCheckRef(limits, limits_polarity, config->final, 0.0, pars->deceleration, meta)))
        {
            meta->error.index = 2;
            return(fg_error);
        }
    }

    return(FG_OK);
}



bool fgRampGen(struct fg_ramp_pars *pars, const double *time, float *ref)
{
    bool        func_running = true;        // Returned value
    uint32_t    time_shift_alg    = 0;      // Time shift adjustment algorithm index
    float       r;
    float       ref_rate_limit;             // Limit on ref due to rate limit
    float       period;                     // Time period calculated using prev_time
    double      ref_time;                   // Time within the segment in seconds

    // Pre-function coast

    if(*time <= pars->delay)
    {
        r = pars->init_ref;
    }
    else
    {
        // If reference received from previous iteration was changed, and isn't blocked

        if(*ref != pars->prev_ramp_ref && *ref != pars->prev_returned_ref)
        {
            // Identify time shift adjustment algorithm according to ramp direction

            if(pars->pos_ramp_flag)
            {
                // Positive (rising) ramp 

                if(*ref > pars->ref[0])
                {
                    if(pars->pre_ramp_flag)
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
                    if(pars->pre_ramp_flag)
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

            // Clear pre_ramp_flag once the main part of the ramp is started

            if(ref_time >= 0.0)
            {
                pars->pre_ramp_flag = 0;
            }
        }

        // Parabolic deceleration

        else if(ref_time < pars->time[2])
        {
            ref_time -= pars->time[2];        // ref_time is relative to end of parabola (negative)
            r = pars->ref[2] + 0.5 * pars->deceleration * ref_time * ref_time;
        }

        // Coast

        else
        {
            r = pars->ref[2];

            // End of function
 
            func_running = false;
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

    return(func_running);
}



void fgRampCalc(struct fg_ramp_config *config,
                double                 delay,
                float                  init_ref,
                float                  init_rate,
                struct fg_ramp_pars   *pars,
                struct fg_meta        *meta)
{
    float           delta_ref;              // Initial ref minus final ref
    float           ref0;                   // Ref at t=0 for first parabola
    float           overshoot_rate_limit;   // Limiting initial rate of change before overshoot occurs
    float           seg_ratio;              // Ratio between the two segments
    struct fg_meta  local_meta;             // Local meta data in case user meta is NULL

    // Prepare meta structure

    meta = fgResetMeta(meta, &local_meta, init_ref);

    // Prepare variables assuming ascending (positive) ramp

    pars->delay             =  delay;
    pars->prev_time         =  delay;
    pars->pos_ramp_flag     =  1;
    pars->acceleration      =  fabs(config->acceleration);
    pars->deceleration      = -fabs(config->deceleration);
    pars->linear_rate       =  fabs(config->linear_rate);
    pars->linear_rate_limit =  fabs(init_rate);
    pars->prev_ramp_ref     =  pars->prev_returned_ref = pars->init_ref = init_ref;
    delta_ref               =  config->final - init_ref;
    overshoot_rate_limit    =  sqrt(-2.0 * pars->deceleration * fabs(delta_ref));

    // Set up accelerations according to ramp direction and possible overshoot

    if((delta_ref >= 0.0 && init_rate >   overshoot_rate_limit) ||
       (delta_ref <  0.0 && init_rate >= -overshoot_rate_limit))
    {
        // Descending (negative) ramp
        
            pars->pos_ramp_flag =  0;
            pars->acceleration  = -pars->acceleration;
            pars->deceleration  = -pars->deceleration;
    }

    // Set time_shift and ref0 and delta_ref to take into account the initial rate of change

    pars->time_shift    = -init_rate / pars->acceleration;
    pars->pre_ramp_flag = (pars->time_shift > 0.0);

    ref0      = init_ref + 0.5 * init_rate * pars->time_shift;
    delta_ref = config->final - ref0;

    // Calculate ramp parameters

    seg_ratio = pars->deceleration / (pars->deceleration - pars->acceleration);

    pars->time[0] = 0.0;
    pars->time[2] = sqrt(2.0 * delta_ref / (seg_ratio * pars->acceleration));
    pars->time[1] = pars->time[2] * seg_ratio;

    pars->ref[0]  = ref0;
    pars->ref[1]  = ref0 + delta_ref * seg_ratio;
    pars->ref[2]  = config->final;

    // Set duration if rate limit is never reached

    meta->duration = pars->time[2] + delay + pars->time_shift;

    // Set min/max

    fgSetMinMax(meta,init_ref);
    fgSetMinMax(meta,config->final);

    // If time_shift is positive then include point of inflexion of first parabola in min/max check

    if(pars->time_shift > 0.0)
    {
        fgSetMinMax(meta,ref0);
    }

    // Complete meta data

    meta->range.end = config->final;
}
// EOF

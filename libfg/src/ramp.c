/*---------------------------------------------------------------------------------------------------------*\
  File:     ramp.c                                                                      Copyright CERN 2014

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

  Purpose:  Generate RAMP function based on Parabola - Parabola with time shift when value is limited

  Notes:    RAMP is a special function that responds by adjusting a time shift if the reference is 
            rate limited by the calling application. This effectively slows the reference time so that
            the function will continue smoothly when the reference is no longer limited. As a consequence,
            the function duration returned in *meta will be wrong if the reference is limited at any 
            time during the generation of the function. In addition, time must never go backwards.
\*---------------------------------------------------------------------------------------------------------*/

#include "libfg/ramp.h"
#include <stdio.h>

/*---------------------------------------------------------------------------------------------------------*/
enum fg_error fgRampInit(struct fg_limits          *limits,
                         enum   fg_limits_polarity  limits_polarity,
                         struct fg_ramp_config     *config,
                         float                      delay,
                         float                      ref,
                         struct fg_ramp_pars       *pars,
                         struct fg_meta            *meta)          // NULL if not required
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;                 // Reference limits status
    uint32_t      negative_flag;            // Flag to limits check function that part of ref is negative

    fgResetMeta(meta);                      // Reset meta structure

    // Check that parameters are valid

    if(config->acceleration <= 0.0 || config->deceleration <= 0.0)
    {
        return(FG_BAD_PARAMETER);
    }

    // Calculate ramp parameters always with zero initial ramp rate

    fgRampCalc(config, pars, delay, ref, 0.0, meta);

    // Check limits if supplied

    if(limits)
    {
        negative_flag = ref < 0.0 || config->final < 0.0;

        // Check limits at the start of the parabolic acceleration (segment 1)

        if((fg_error = fgCheckRef(limits, limits_polarity, negative_flag, ref,
                                 0.0, pars->acceleration, meta)))
        {
            meta->error.index = 1;
            return(fg_error);
        }

        // Check limits at the end of the parabolic deceleration (segment 2)

        if((fg_error = fgCheckRef(limits, limits_polarity, negative_flag, config->final,
                                 0.0, pars->deceleration, meta)))
        {
            meta->error.index = 2;
            return(fg_error);
        }
    }

    return(FG_OK);                              // Report success
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t fgRampGen(struct fg_ramp_pars *pars, const double *time, float *ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function derives the reference for the previously initialised ramp function at the given time.
  It returns zero if time is beyond the end of the function, and 1 otherwise. *ref should point to the
  the reference value from the previous iteration. This may have been clipped if any limits were reached.

  The input pars structure contains the coordinates of the transition points between the segments of the
  ramp function: 

   - pars->time[0], pars->ref[0]: Max/min of first (accelerating) parabola;
   - pars->time[1], pars->ref[1]: Connection between acceleating and decelerating parabolas;
   - pars->time[2], pars->ref[2]: End of the second (decelerating) parabola, also end of the ramp function.
  
  NOTE: Unlike the other libfg functions, TIME MUST NOT GO BACKWARDS. 
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    func_running_flag = 1;      // Returned value
    uint32_t    time_shift_alg    = 0;      // Time shift adjustment algorithm index
    float       r;
    float       ref_rate_limit;             // Limit on ref due to rate limit
    double      ref_time;                   // Time within the segment in seconds

    // NB: In the common case where time == 0.0, pars->delay == 0.0 and the initial rate is != 0.0 then the
    //     reference must be calculated based on the 1st parabola and not the plateau below. This is the
    //     reason why a strict comparison operator is used.

    if(*time < pars->delay)
    {
        r = pars->ref[0];
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
 
            func_running_flag = 0;
        }
    }

    // Keep ramp reference for next iteration (before rate limiter)

    pars->prev_ramp_ref = r;

    // Apply rate limit if active

    if(pars->linear_rate > 0.0)
    {
        if(++pars->iteration_idx == 2)
        {
            pars->period = *time - pars->prev_time;
        }
        else if(pars->iteration_idx > 2)
        {
            if(r > pars->prev_returned_ref)
            {
                // Positive rate of change

                ref_rate_limit = *ref + pars->linear_rate * pars->period;

                if(r > ref_rate_limit)
                {
                    r = ref_rate_limit;
                }
            }
            else if(r < pars->prev_returned_ref)
            {
                // Negative rate of change

                ref_rate_limit = *ref - pars->linear_rate * pars->period;

                if(r < ref_rate_limit)
                {
                    r = ref_rate_limit;
                }
            }
        }
    }

    // Keep returned reference and time for next iteration

    pars->prev_returned_ref = *ref;
    pars->prev_time         = *time;

    // Return new reference after rate limit

    *ref = r;

    return(func_running_flag);
}
/*---------------------------------------------------------------------------------------------------------*/
static void fgRampSetMinMax(struct fg_meta *meta, float ref)
/*---------------------------------------------------------------------------------------------------------*\
  This helper function is used by fgRampCalc() to set the meta min and max fields
\*---------------------------------------------------------------------------------------------------------*/
{
    if(meta->range.min == 0.0 && meta->range.max == 0.0)
    {
        meta->range.min = meta->range.max = ref;
    }
    else
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
}
/*---------------------------------------------------------------------------------------------------------*/
void fgRampCalc(struct fg_ramp_config *config,
                struct fg_ramp_pars   *pars,
                float                  delay,
                float                  init_ref,
                float                  init_rate,
                struct fg_meta        *meta)            // can be NULL if not required
/*---------------------------------------------------------------------------------------------------------*\
  This function calculates ramp parameters. 

  Please refer to the comments in function fgRampGen for information regarding the contents of the output
  pars structure, and in particular the usage of pars->time[], pars->ref[].
\*---------------------------------------------------------------------------------------------------------*/
{
    float       delta_ref;              // Initial ref minus final ref
    float       ref0;                   // Ref at t=0 for first parabola
    float       overshoot_rate_limit;   // Limiting initial rate of change before overshoot occurs
    float       seg_ratio;              // Ratio between the two segments

    // Prevent non-zero delay and init_rate - this would imply a bug in calling program

    if(delay != 0.0 && init_rate != 0.0)
    {
        delay = 0.0;
    }

    // Prepare variables

    pars->delay          = delay;
    pars->linear_rate    = config->linear_rate;
    pars->prev_ramp_ref  = pars->prev_returned_ref = init_ref;
    pars->iteration_idx  = 0;
    delta_ref            = config->final - init_ref;
    overshoot_rate_limit = sqrt(2.0 * config->deceleration * fabs(delta_ref));

    // Set up accelerations according to ramp direction

    if(delta_ref >= 0.0)
    {
        // Positive (rising) ramp

        if(init_rate > overshoot_rate_limit)
        {
            // Positive ramp overshoots so becomes negative

            pars->pos_ramp_flag =  0;
            pars->acceleration  = -config->deceleration;
            pars->deceleration  =  config->deceleration;
        }
        else
        {
            pars->pos_ramp_flag =  1;
            pars->acceleration  =  config->acceleration;
            pars->deceleration  = -config->deceleration;
        }
    }
    else
    {                                        
        // Negative (falling) ramp
        
        if(init_rate < -overshoot_rate_limit)
        {
            // Negative ramp overshoots so becomes positive

            pars->pos_ramp_flag =  1;
            pars->acceleration  =  config->deceleration;
            pars->deceleration  = -config->deceleration;
        }
        else
        {
            pars->pos_ramp_flag =  0;
            pars->acceleration  = -config->acceleration;
            pars->deceleration  =  config->deceleration;
        }
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

    // Return meta data

    if(meta != NULL)
    {
        meta->range.start = init_ref;
        meta->range.end   = config->final;

        // Set duration if rate limit never reached

        meta->duration = pars->time[2] + delay + pars->time_shift;  

        // Set min/max 

        fgRampSetMinMax(meta,init_ref);
        fgRampSetMinMax(meta,config->final);

        // If time_shift is positive then include point of inflexion of first parabola in min/max check

        if(pars->time_shift > 0.0)
        {
            fgRampSetMinMax(meta,ref0);
        }
    }
}
// EOF


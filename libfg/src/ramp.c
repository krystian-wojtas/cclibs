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

    // Calculate ramp parameters 

    fgRampCalc(config, pars, delay, ref, meta);

    // Check limits if supplied

    if(limits)
    {
        negative_flag = ref < 0.0 || config->final < 0.0;

        // Check limits at the end of the parabolic acceleration (segment 1)

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

   - pars->time[0], pars->ref[0]: Start of the first (accelerating) parabola;
   - pars->time[1], pars->ref[1]: Connection between acceleating and decelerating parabolas;
   - pars->time[2], pars->ref[2]: End of the second (decelerating) parabola, also end of the ramp function.
  
  NOTE: Unlike the other libfg functions, TIME MUST NOT GO BACKWARDS. 
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    func_running_flag = 1;      // Returned value
    float       r;
    double      ref_time;                   // Time within the segment in seconds

    // Pre-acceleration coast

    if(*time < pars->delay)
    {
        r = pars->ref[0];
    }
    else
    {
        // If reference received from previous iteration was changed, and isn't blocked

        if(*ref != pars->prev_ramp_ref && *ref != pars->prev_returned_ref)
        {
            // Calculate new time shift according to ramp direction

            if(pars->pos_ramp_flag)
            {
                // Positive (rising) ramp 

                if(*ref <= pars->ref[0])
                {
                    pars->time_shift = 0.0;
                }
                else if(*ref <= pars->ref[1])
                {
                     pars->time_shift = pars->prev_time - pars->delay -
                                        sqrt(2.0 * (*ref - pars->ref[0]) / pars->acceleration); // acceleration always +ve
                }
                else if(*ref <= pars->ref[2])
                {
                     pars->time_shift = pars->prev_time - pars->delay -
                                        (pars->time[2] - sqrt(2.0 * (*ref - pars->ref[2]) / pars->deceleration)); // deceleration always -ve
                }
            }
            else // Negative (falling) ramp
            {
                if(*ref >= pars->ref[0])
                {
                    pars->time_shift = 0.0;
                }
                else if(*ref >= pars->ref[1])
                {
                     pars->time_shift = pars->prev_time - pars->delay -
                                        sqrt(2.0 * (*ref - pars->ref[0]) / pars->acceleration); // acceleration always -ve
                }
                else if(*ref >= pars->ref[2])
                {
                     pars->time_shift = pars->prev_time - pars->delay -
                                        (pars->time[2] - sqrt(2.0 * (*ref - pars->ref[2]) / pars->deceleration)); // deceleration always +ve
                }
            }
        }

        // Calculate new ref_time including delay and time_shift

        ref_time = *time - pars->delay - pars->time_shift;

        // Parabolic acceleration

        if(ref_time <= pars->time[1])
        {
            r = pars->ref[0] + 0.5 * pars->acceleration * ref_time * ref_time;
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

    // Keep returned reference for next iteration

    pars->prev_returned_ref = *ref;

    // Keep time and new ramp reference for next iteration

    pars->prev_time     = *time;
    pars->prev_ramp_ref = *ref = r;

    return(func_running_flag);
}
/*---------------------------------------------------------------------------------------------------------*/
void fgRampCalc(struct fg_ramp_config *config,
              struct fg_ramp_pars     *pars,
              float                    delay,
              float                    init_ref,
              struct fg_meta          *meta)            // can be NULL if not required
/*---------------------------------------------------------------------------------------------------------*\
  This function calculates ramp parameters. 

  Please refer to the comments in function fgRampGen for information regarding the contents of the output
  pars structure, and in particular the usage of pars->time[], pars->ref[].
\*---------------------------------------------------------------------------------------------------------*/
{
    float       delta_ref;              // Initial ref minus final ref
    float       seg_ratio;              // Ratio between the two segments

    // Prepare variables

    pars->time_shift    = 0.0;
    pars->delay         = delay;
    pars->prev_ramp_ref = pars->prev_returned_ref = init_ref;
    delta_ref           = config->final - init_ref;

    // Set up accelerations according to ramp direction

    if(delta_ref >= 0.0)
    {
        // Positive (rising) ramp
        
        pars->pos_ramp_flag = 1;
        pars->acceleration  =  config->acceleration;
        pars->deceleration  = -config->deceleration;
    }
    else
    {                                        
        // Negative (falling) ramp
        
        pars->pos_ramp_flag = 0;
        pars->acceleration  = -config->acceleration;
        pars->deceleration  =  config->deceleration;
    }

    // Calculate ramp parameters

    seg_ratio = pars->deceleration / (pars->deceleration - pars->acceleration);

    pars->time[0] = 0.0;
    pars->time[2] = sqrt(2.0 * delta_ref / (seg_ratio * pars->acceleration));
    pars->time[1] = pars->time[2] * seg_ratio;

    pars->ref[0]  = init_ref;
    pars->ref[1]  = init_ref + delta_ref * seg_ratio;
    pars->ref[2]  = config->final;

    // Return meta data

    if(meta != NULL)
    {
        meta->duration    = pars->time[2] + delay;  // Duration if rate limit never reached
        meta->range.start = init_ref;
        meta->range.end   = config->final;

        if(pars->pos_ramp_flag)                     // Set min/max according to positive/negative ramp
        {
            meta->range.min = meta->range.start;
            meta->range.max = meta->range.end;
        }
        else
        {
            meta->range.min = meta->range.end;
            meta->range.max = meta->range.start;
        }
    }
}
// EOF


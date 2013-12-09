/*---------------------------------------------------------------------------------------------------------*\
  File:     pp.c                                                                      Copyright CERN 2014

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

  Purpose:  Generate Parabola - Parabola (PP) function with time shift when limited

  Notes:    This is a special function that responds if the reference is rate limited by the calling
            application by adjusting a time shift. This effectively slows the reference time so that
            the function will continue smoothly when the reference is no longer limited.
\*---------------------------------------------------------------------------------------------------------*/

#include "libfg/pp.h"

/*---------------------------------------------------------------------------------------------------------*/
enum fg_error fgPpInit(struct fg_limits        *limits,
                       enum fg_limits_polarity  limits_polarity,
                       struct fg_pp_config     *config,
                       float                    delay,
                       float                    ref,
                       struct fg_pp_pars       *pars,
                       struct fg_meta          *meta)          // NULL if not required
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;                     // Limits status
    uint32_t      negative_flag;                // Flag to limits check function that part of ref is negative

    fgResetMeta(meta);                          // Reset meta structure

    // Check parameters are valid

    if(config->acceleration <= 0.0)
    {
        return(FG_BAD_PARAMETER);
    }

    // Calculate pp parameters 

    fgPpCalc(config, pars, delay, ref, meta);

    // Check limits if supplied

    if(limits)
    {
        negative_flag = ref < 0.0 || config->final < 0.0;

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
uint32_t fgPpGen(struct fg_pp_pars *pars, const double *time, float *ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function derives the reference for the previously initialised pp function at the given time.
  It returns zero if time is beyond the end of the function, and 1 otherwise.

  The input pars structure contains the coordinates of the transition points between the segments of the
  pp function.  The coordinates are defined for a normalised, descending, pp function. The reference must be
  adjusted (de-normalised) in fgPpGen if the PP is ascending, namely if pars->pos_ramp_flag == TRUE.

   - pars->time[0], pars->ref[0]: Start of the first (accelerating) parabola
   - pars->time[1], pars->ref[1]: Connection between acceleating and decelerating parabolas;
   - pars->time[2], pars->ref[2]: End of the second (decelerating) parabola, also the end of the pp function.
  
  Unlike the other functions, time must not go backwards.
\*---------------------------------------------------------------------------------------------------------*/
{
    float       r;
    double      ref_time;                 // Time within the segment in seconds

    // Pre-acceleration coast

    if(*time < pars->delay)
    {
        r = pars->ref[0];
    }
    else
    {
        // If reference received from previous iteration was changed, but isn't blocked

        if(*ref != pars->prev_pp_ref && *ref != pars->prev_returned_ref)
        {
            // Normalise reference received for previous iteration

            if(!pars->pos_ramp_flag)
            {
                r = *ref;
            }
            else
            {
                r = pars->offset - *ref;
            }

            // Identify which segment the received reference is in and update time_shift
            
            if(r >= pars->ref[0])
            {
                pars->time_shift = 0.0;
            }
            else if(r >= pars->ref[1])
            {
                 pars->time_shift = pars->prev_time - pars->delay -
                                    sqrt(2.0 * (r - pars->ref[1]) / pars->acceleration); // acceleration always +ve
            }
            else if(r >= pars->ref[2])
            {
                 pars->time_shift = pars->prev_time - pars->delay -
                                    (pars->time[2] - sqrt(2.0 * (r - pars->ref[2]) / pars->deceleration)); // deceleration always +ve
            }
        }

        // Calculate new ref_time including delay and time_shift

        ref_time = *time - pars->delay - pars->time_shift;

        // Parabolic acceleration

        if(ref_time <= pars->time[1])
        {
            r = pars->ref[0] - 0.5 * pars->acceleration * ref_time * ref_time;
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
            pars->prev_returned_ref = *ref;

            // End of function
 
            *ref = pars->ref[2];

            pars->prev_pp_ref = *ref;
            pars->prev_time   = *time;
           
            return(0);                       // Report that function has finished
        }
    }

    pars->prev_returned_ref = *ref;

    // Invert if ramp is positive

    if(!pars->pos_ramp_flag)
    {
        *ref = r;
    }
    else
    {
        *ref = pars->offset - r;
    }

    // Keep time and ref for next iteration

    pars->prev_pp_ref = *ref;
    pars->prev_time   = *time;

    return(1);                  // Report that function is still running
}
/*---------------------------------------------------------------------------------------------------------*/
void fgPpCalc(struct fg_pp_config *config,
              struct fg_pp_pars   *pars,
              float                delay,
              float                init_ref,
              struct fg_meta       *meta)            // can be NULL if not required
/*---------------------------------------------------------------------------------------------------------*\
  This function calculates pp parameters.  This function must be re-entrant because it can be called
  to calculate the pp coefficients for an already moving reference.

  The reference scale is normalised if the pp is ascending.  This reflects the curve about the final
  reference value (pp->ref[2]) to make the calculated pp always descending.

  Please refer to the comments in function fgPpGen for information regarding the contents of the output
  pars structure, and in particular the usage of pars->time[i], pars->ref[i] for all indexes 0 to 2.
\*---------------------------------------------------------------------------------------------------------*/
{
    float       delta_ref;              // Initial ref minus final ref
    float       pp_ratio;               // Ratio between the two segments

    // Prepare variables

    pars->acceleration = config->acceleration;
    pars->deceleration = config->acceleration;  // For now, the deceleration equals acceleration
    pars->delay        = delay;
    pars->time_shift   = 0.0;
    pars->prev_pp_ref  = pars->prev_returned_ref = init_ref;

    delta_ref = init_ref - config->final;       // Total reference change

    // Normalise if pp is ascending

    if(delta_ref >= 0.0)                       // Descending pp
    {
        pars->pos_ramp_flag = 0;
    }
    else                                        // Ascending pp
    {
        pars->pos_ramp_flag = 1;
        pars->offset        = 2.0 * config->final;
        init_ref            = pars->offset - init_ref;          // Normalised initial reference
        delta_ref           = -delta_ref;                       // Normalised delta
    }

    // Calculate pp parameters

    pp_ratio = pars->deceleration / (pars->acceleration + pars->deceleration);

    pars->time[0] = 0.0;
    pars->time[2] = sqrt(2.0 * pars->acceleration * delta_ref / pp_ratio);
    pars->time[1] = pars->time[2] * pp_ratio;

    pars->ref[0]  = init_ref;
    pars->ref[1]  = init_ref - delta_ref * pp_ratio;
    pars->ref[2]  = config->final;

    // Return meta data

    if(meta != NULL)
    {
        meta->duration  = pars->time[2];  // Duration if rate limit never reached
        meta->range.end = config->final;

        if(pars->pos_ramp_flag)              // If final reference is greater than initial reference
        {
            meta->range.min = meta->range.start = pars->offset - init_ref;      // De-normalise to retrieve the initial reference
            meta->range.max = meta->range.end;
        }
        else
        {
            meta->range.min = meta->range.end;
            meta->range.max = meta->range.start = init_ref;
        }
    }
}
// EOF

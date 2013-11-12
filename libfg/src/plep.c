/*---------------------------------------------------------------------------------------------------------*\
  File:     plep.c                                                                      Copyright CERN 2011

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

  Purpose:  Generate Parabola - Linear - Expential - Parabola (PLEP) functions

  Notes:    A PLEP can have up to four segments: Parabola - Linear - Expential - Parabola
            The exponential is only required when ramping down a 1-quadrant converter.
            The normalised PLEP is always calculated as a negative going function.  If the PLEP is
            positive then it is reflected around the final value.  The PLEP can start with a
            non-zero rate of change.  The function is defined by ref[5] and time[5] arrays
            in the parameters structure.  These contain the segment times and normalised values.

            See PLEPn.pdf in the doc directory for details
\*---------------------------------------------------------------------------------------------------------*/

#include "libfg/plep.h"

/*---------------------------------------------------------------------------------------------------------*/
enum fg_error fgPlepInit(struct fg_limits       *limits,
                         enum fg_limits_polarity limits_polarity,
                         struct fg_plep_config  *config,
                         float                   delay,
                         float                   ref,
                         struct fg_plep_pars    *pars,
                         struct fg_meta         *meta)          // NULL if not required
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;                     // Limits status
    uint32_t      i;                            // Loop variable
    uint32_t      negative_flag;                // Flag to limits check function that part of ref is negative
    float         inv;                          // Ramp direction factor: +/-1.0
    float         abs_ref[FG_PLEP_N_SEGS];      // Absolute reference values (i.e. not normalised)

    fgResetMeta(meta);                          // Reset meta structure

    // Check parameters are valid

    if(config->acceleration <= 0.0 || config->linear_rate <= 0.0 ||
       (config->exp_tc > 0.0 && config->exp_final >= config->final))
    {
        return(FG_BAD_PARAMETER);
    }

    // Calculate PLEP parameters with zero initial rate of change

    fgPlepCalc(config, pars, delay, ref, 0.0, meta);

    // Check limits if supplied

    if(limits)
    {
        negative_flag = ref < 0.0 || config->final < 0.0;
        inv           = (pars->pos_ramp_flag ? -1.0 : 1.0);

        // Calculate unnormalised reference values for all segments

        for(i = 0 ; i < FG_PLEP_N_SEGS ; i++)
        {
            if(pars->pos_ramp_flag)
            {
                abs_ref[i] = pars->offset - pars->ref[i + 1];
            }
            else
            {
                abs_ref[i] = pars->ref[i + 1];
            }
        }

        // Check limits at the end of the parabolic acceleration (segment 1)

        if((fg_error = fgCheckRef(limits, limits_polarity, negative_flag, abs_ref[0],
                                 (inv * pars->acceleration * (pars->time[1] - pars->time[0])),
                                 pars->acceleration, meta)))
        {
            meta->error.index = 1;
            return(fg_error);
        }

        // Check limits at the end of the linear segment (segment 2)

        if(pars->time[2] > pars->time[1] &&
           (fg_error = fgCheckRef(limits, limits_polarity, negative_flag, abs_ref[1],
                                 inv * pars->linear_rate,
                                 pars->acceleration, meta)))
        {
            meta->error.index = 2;
            return(fg_error);
        }

        // Check limits at the end of the exponential decay (segment 3)

        if((fg_error = fgCheckRef(limits, limits_polarity, negative_flag, abs_ref[2],
                                 (inv * pars->acceleration * (pars->time[4] - pars->time[3])),
                                 pars->acceleration, meta)))
        {
            meta->error.index = 3;
            return(fg_error);
        }

        // Check limits at the end of the parabolic deceleration (segment 4)

        if((fg_error = fgCheckRef(limits, limits_polarity, negative_flag, abs_ref[3],
                                 0.0, pars->acceleration, meta)))
        {
            meta->error.index = 4;
            return(fg_error);
        }
    }

    return(FG_OK);                              // Report success
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t fgPlepGen(struct fg_plep_pars *pars, const double *time, float *ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function derives the reference for the previously initialised PLEP function at the given time.
  It returns zero if time is beyond the end of the function, and 1 otherwise.

  The input pars structure contains the coordinates of the transition points between the segments of the
  PLEP function, except for the point at index 0 which is used slightly differently (see details below).

  Finally, the coordinates are defined for a normalised, descending, PLEP function. The reference must be
  adjusted (de-normalised) in fgPlepGen if the PLEP is ascending, namely if pars->pos_ramp_flag == TRUE.

   - pars->time[0], pars->ref[0]: TOP of the first parabola (NOT the beginning of the first parabola);
   - pars->time[1], pars->ref[1]: End of the first parabola;
   - pars->time[2], pars->ref[2]: End of the linear segment;
   - pars->time[3], pars->ref[3]: End of the exponential segments;
   - pars->time[4], pars->ref[4]: End of the second parabola, also the end of the PLEP function.

\*---------------------------------------------------------------------------------------------------------*/
{
    float       r;
    double      ref_time;                 // Time within the segment in seconds

    // Pre-acceleration coast - only possible if initial rate is zero

    // NB: In the common case where time == 0.0, pars->delay == 0.0 and the initial rate is != 0.0 then the
    //     reference must be calculated based on the 1st parabola and not the plateau below. This is the
    //     reason why a strict comparison operator is used.

    if(*time < pars->delay)
    {
        r = pars->ref[0];
    }

    // Parabolic

    else if(*time <= pars->time[1])
    {
        ref_time = *time - pars->time[0];        // ref_time is relative to min/max of parabola
        r        = pars->ref[0] + 0.5 * pars->acceleration * ref_time * ref_time;
    }

    // Linear ramp

    else if(*time <= pars->time[2])
    {
        ref_time = *time - pars->time[1];        // ref_time is relative to start of linear segment
        r        = pars->ref[1] + pars->linear_rate * ref_time;
    }

    // Exponential deceleration

    else if(*time <= pars->time[3])
    {
        ref_time = *time - pars->time[2];        // ref_time is relative to start of exponential segment
        r        = pars->ref_exp * exp(pars->inv_exp_tc * ref_time) + pars->exp_final;
    }

    // Parabolic deceleration

    else if(*time < pars->time[4])
    {
        ref_time = *time - pars->time[4];        // ref_time is relative to end of parabola (negative)
        r        = pars->ref[4]  - 0.5 * pars->acceleration * ref_time * ref_time;
    }

    // Coast

    else
    {
        // End of function

       *ref = pars->ref[4];
       return(0);                       // Report that function has finished
    }

    // Invert if ramp is positive

    if(!pars->pos_ramp_flag)
    {
        *ref = r;
    }
    else
    {
        *ref = pars->offset - r;
    }

    return(1);                  // Report that function is still running
}
/*---------------------------------------------------------------------------------------------------------*/
void fgPlepCalc(struct fg_plep_config *config,
                struct fg_plep_pars   *pars,
                float                  delay,
                float                  init_ref,
                float                  init_rate,
                struct fg_meta        *meta)            // can be NULL if not required
/*---------------------------------------------------------------------------------------------------------*\
  This function calculates PLEP parameters.  This function must be re-entrant because it can be called
  to calculate the PLEP coefficients for an already moving reference.

  The reference scale is normalised if the PLEP is ascending.  This reflects the curve about the final
  reference value (plep->ref[4]) to make the calculated PLEP always descending.

  Please refer to the comments in function fgPlepGen for information regarding the contents of the output
  pars structure, and in particular the usage of pars->time[i], pars->ref[i] for all indexes 0 to 4.

  Note that if delay is non-zero then init_rate should be zero.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    i;                      // Loop variable
    uint32_t    par_b4_lin_flag;        // Decelerating parabola before Linear flag
    uint32_t    par_b4_exp_flag;        // Decelerating parabola before Exponential flag
    uint32_t    exp_b4_lin_flag;        // Exponential before Linear flag
    uint32_t    exp_f;                  // Exponential segment present flag
    float       delta_ref;              // Initial ref minus final ref
    float       min_exp_final;          // Minimum allowable exp_final
    float       exp_rate;               // Initial rate of change for exponential segment
    float       delta_time1_par;        // Time between the top of first parabolic segment and the start of the second one
    float       delta_time1_lin;        // Time between the top of first parabolic segment and the start of linear segment
    float       delta_time1_exp;        // Time until exponential segment
    float       delta_time[5];          // Segment durations
    float       ref_time;               // Segment time accumulator
    float       inv_acc;                // 1 / pars->acceleration

    // Prepare variables

    pars->acceleration = config->acceleration;
    pars->linear_rate  = config->linear_rate;
    pars->delay        = delay;
    pars->ref[4]       = config->final;

    delta_ref = init_ref - config->final;       // Total reference change
    inv_acc   = 1.0 / pars->acceleration;       // Inverse acceleration

    delta_time[1] = delta_time[2] = delta_time[3] = 0.0;        // Clear segment times

    // Normalise if PLEP is ascending

    if(delta_ref >= 0.0)                       // Descending PLEP
    {
        exp_f               = (config->exp_tc > 0.0);   // Exponential segment enabled if exp_tc > 0
        pars->pos_ramp_flag = 0;
    }
    else                                        // Ascending PLEP
    {
        exp_f               = 0;                        // No exponential allowed for ascending PLEP
        pars->pos_ramp_flag = 1;
        pars->offset        = 2.0 * config->final;
        init_ref            = pars->offset - init_ref;          // Normalised initial reference
        init_rate           = -init_rate;                       // Normalised initial rate
        delta_ref           = -delta_ref;                       // Normalised delta
    }

    // Prepare for exponential segment

    if(exp_f)
    {
        pars->inv_exp_tc = -1.0 / config->exp_tc;

        min_exp_final = config->final - 0.5 * pars->acceleration * config->exp_tc * config->exp_tc;

        if(config->exp_final < min_exp_final)           // Clip exp final at minimum permitted value
        {
            pars->exp_final = min_exp_final;
        }
        else
        {
            pars->exp_final = config->exp_final;
        }

        exp_rate = (pars->exp_final - init_ref) * pars->inv_exp_tc;  // Calc rate at start of exp segment

        if(exp_rate < pars->linear_rate)                // Clip linear rate of change to exp limit
        {
            pars->linear_rate = exp_rate;
        }
    }

    // Clip actual rate of change to rate limit (just in case)

    if(init_rate < -pars->linear_rate)                          // Clip initial rate to maximum value
    {
        init_rate = -pars->linear_rate;
    }

    // Calculate PLEP parameters

    // Case 1 : Inverted P-P

    if(init_rate < -sqrt(2.0 * pars->acceleration * delta_ref))
    {
        float dt1_squared;

        delta_time[0] = -init_rate * inv_acc;

        pars->ref[0] = init_ref - 0.5 * init_rate * init_rate * inv_acc;

        dt1_squared = inv_acc * (pars->ref[4] - pars->ref[0]);

        if(dt1_squared > 0.0)   // Protect sqrt() against negative values due to float rounding errors
        {
            delta_time[1] = sqrt(dt1_squared);
        }

        delta_time[4] = delta_time[1];

        pars->ref[1] = pars->ref[2] = pars->ref[3] = 0.5 * (pars->ref[4] - pars->ref[0]);

        goto end;
    }

    // Case 2, 3, 4, 5: P-P, P-E-P, P-L-P, P-L-E-P

    pars->linear_rate   = -pars->linear_rate;   // Always strictly negative
    pars->acceleration  = -pars->acceleration;  // Always strictly negative
    inv_acc             = -inv_acc;             // Always strictly negative

    // Delta time and reference of the TOP of the first parabola (NB: the delta time can be negative)

    delta_time[0] = -init_rate * inv_acc;
    pars->ref[0] = init_ref - 0.5 * init_rate * init_rate * inv_acc;

    // Delta times between the TOP of the first parabola and either the start of the second parabola (P-P, P-E-P cases)
    // or the start of the linear segment (P-L-P, P-L-E-P cases)

    delta_time1_par = sqrt(inv_acc * (pars->ref[4] - pars->ref[0]));
    delta_time1_lin = pars->linear_rate * inv_acc;

    if(exp_f)
    {
        delta_time1_exp = sqrt(config->exp_tc * config->exp_tc +
                               2.0 * inv_acc * (pars->exp_final - pars->ref[0])) - config->exp_tc;
    }
    else
    {
        delta_time1_exp = 1.0E30;
    }

    par_b4_lin_flag = (delta_time1_par < delta_time1_lin);
    par_b4_exp_flag = (delta_time1_par < delta_time1_exp);
    exp_b4_lin_flag = (delta_time1_exp < delta_time1_lin);

    // Case 2 : P-P

    if(par_b4_lin_flag && par_b4_exp_flag)
    {
        delta_time[1] = delta_time[4] = delta_time1_par;
        pars->ref[1] = pars->ref[2] = pars->ref[3] = 0.5 * (pars->ref[0] + pars->ref[4]);

        goto end;
    }

    // Case 2 : P-E-P

    if(exp_b4_lin_flag)
    {
        delta_time[1] = delta_time1_exp;
        pars->ref[1] = pars->ref[2] = pars->ref[0] + 0.5 * pars->acceleration * delta_time[1] * delta_time[1];

        exp_par:        // From goto exp_par;

        delta_time[4] = config->exp_tc - sqrt(config->exp_tc * config->exp_tc +
                                              2.0 * inv_acc * (pars->ref[4] - pars->exp_final));

        delta_time[3] = -config->exp_tc * log((pars->ref[4] - pars->exp_final -
                                               0.5 * pars->acceleration * delta_time[4] * delta_time[4]) /
                                              (pars->ref[2] - pars->exp_final));

        pars->ref[3]    = pars->ref[4] - 0.5 * pars->acceleration * delta_time[4] * delta_time[4];
        pars->ref_exp   = pars->ref[2] - pars->exp_final;

        goto end;
    }

    // Case 3, 4 : P-L-P or P-L-E-P

    delta_time[1] = delta_time1_lin;
    pars->ref[1] = pars->ref[0] + 0.5 * pars->acceleration * delta_time[1] * delta_time[1];

    pars->ref[3] = pars->ref[4] - 0.5 * inv_acc * pars->linear_rate * pars->linear_rate;

    if(exp_f)
    {
        pars->ref[2] = pars->exp_final - pars->linear_rate * config->exp_tc;

        if(pars->ref[2] > pars->ref[3])
        {
            // Case 4 : P-L-E-P

            delta_time[2] = (pars->ref[2] - pars->ref[1]) / pars->linear_rate;

            goto exp_par;       // part of Case 2: P-E-P above
        }
    }

    // Case 3 : P-L-P

    pars->ref[2]  = pars->ref[3];
    delta_time[2] = (pars->ref[2] - pars->ref[1]) / pars->linear_rate;
    delta_time[4] = pars->linear_rate * inv_acc;

    // Calculate segment times

    end:        // From goto end;

    ref_time = pars->delay;

    for(i = 0 ; i < 5 ; i++)
    {
        ref_time += delta_time[i];
        pars->time[i] = ref_time;
    }

    // Return meta data

    if(meta != NULL)
    {
        meta->duration    = pars->time[4];
        meta->range.end   = config->final;

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
/*---------------------------------------------------------------------------------------------------------*\
  End of file: plep.c
\*---------------------------------------------------------------------------------------------------------*/

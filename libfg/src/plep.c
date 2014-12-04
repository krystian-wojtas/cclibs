/*!
 * @file  plep.c
 * @brief Generate Parabola-Linear-Exponential-Parabola (PLEP) function
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

#include "libfg/plep.h"
#include <stdio.h>

enum fg_error fgPlepInit(struct fg_limits          *limits,
                         enum   fg_limits_polarity  limits_polarity,
                         struct fg_plep_config     *config,
                         double                     delay,
                         float                      init_ref,
                         struct fg_plep_pars       *pars,
                         struct fg_meta            *meta)
{
    enum fg_error  fg_error;                    // Limits status
    struct fg_meta local_meta;                  // Local meta data in case user meta is NULL
    uint32_t       i;                           // Loop variable
    uint32_t       par_b4_lin_flag;             // Decelerating parabola before Linear flag
    uint32_t       par_b4_exp_flag;             // Decelerating parabola before Exponential flag
    uint32_t       exp_b4_lin_flag;             // Exponential before Linear flag
    uint32_t       exp_f;                       // Exponential segment present flag
    float          delta_ref;                   // Initial ref minus final ref
    float          min_exp_final;               // Minimum allowable exp_final
    float          exp_rate;                    // Initial rate of change for exponential segment
    float          delta_time1_par;             // Time between the top of first parabolic segment and the start of the second one
    float          delta_time1_lin;             // Time between the top of first parabolic segment and the start of linear segment
    float          delta_time1_exp;             // Time until exponential segment
    float          ref_time;                    // Segment time accumulator
    float          inv_acc;                     // 1 / pars->acceleration
    float          delta_time[FG_PLEP_N_SEGS+1];// Segment durations

    meta = fgResetMeta(meta, &local_meta, init_ref);  // Reset meta structure - uses local_meta if meta is NULL

    // Check parameters are valid

    if(config->acceleration == 0.0 || config->linear_rate == 0.0 || (config->exp_tc > 0.0 &&
       (config->final == 0.0 || config->final_rate != 0.0 || fabs(config->exp_final) >= fabs(config->final))))
    {
        fg_error = FG_BAD_PARAMETER;
        goto error;
    }

    // Calculate PLEP parameters with zero initial rate of change

    // Prepare variables

    pars->acceleration = fabs(config->acceleration);
    pars->linear_rate  = fabs(config->linear_rate);
    pars->final_rate   = config->final_rate;
    pars->delay        = delay;

    delta_ref = init_ref - config->final;       // Total reference change
    inv_acc   = 1.0 / pars->acceleration;       // Inverse acceleration

    delta_time[0] = delta_time[1] = delta_time[2] = delta_time[3] = 0.0;        // Clear segment times

    // Calculate final accelerating parabola

    if(pars->final_rate >= 0.0)
    {
        pars->final_acc = pars->acceleration;
    }
    else
    {
        pars->final_acc = -pars->acceleration;
    }

    delta_time[5] = pars->final_rate / pars->final_acc;
    pars->ref[4]  = config->final - 0.5 * pars->final_rate * delta_time[5];

    // Allow exponential segment if all conditions are satisfied :

    exp_f = (pars->final_rate == 0.0              &&     // The final rate is zero
             init_ref * config->final >= 0.0      &&     // The PLEP doesn't cross zero
             fabs(init_ref) > fabs(config->final) &&     // The magnitude of the current is decreasing
             config->exp_tc > 0.0);                      // The exponential time constant is greater than zero

    // Normalise if PLEP is ascending by reflecting across ref = 0

    if(delta_ref >= 0.0)                       // Descending PLEP
    {
        pars->normalisation = 1.0;
        pars->ref[5]        = config->final;             // Normalised final reference
    }
    else                                        // Ascending PLEP
    {
        pars->normalisation = -1.0;
        init_ref            = -init_ref;                 // Normalised initial reference
        delta_ref           = -delta_ref;                // Normalised delta
        pars->ref[4]        = -pars->ref[4];             // Normalised penultimate reference
        pars->ref[5]        = -config->final;            // Normalised final reference
        pars->final_rate    = -pars->final_rate;         // Normalised final rate
        pars->final_acc     = -pars->final_acc;          // Normalised final acceleration
    }

    pars->ref[0] = init_ref;

    // Prepare for exponential section if required

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

        // Calculate rate at start of exp segment

        exp_rate = (pars->exp_final - init_ref) * pars->inv_exp_tc;

        // Clip linear rate of change to exp limit

        if(exp_rate < pars->linear_rate)
        {
            pars->linear_rate = exp_rate;
        }
    }

    // Calculate PLEP parameters for different cases: 1.P-P, 2.P-E-P, 3.P-L-P, 4.P-L-E-P

    pars->linear_rate   = -pars->linear_rate;   // Always strictly negative
    pars->acceleration  = -pars->acceleration;  // Always strictly negative
    inv_acc             = -inv_acc;             // Always strictly negative

    // Delta times between the start of the first parabola and either the start of the second
    // parabola (P-P, P-E-P cases) or the start of the linear segment (P-L-P, P-L-E-P cases)

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

    // Case 1 : P-P

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

    // Calculate segment times and min/max reference 

    end:        // From goto end;

    ref_time = 0.0;;

    for(i = 0 ; i <= FG_PLEP_N_SEGS ; i++)
    {
        ref_time     += delta_time[i];
        pars->time[i] = ref_time;

        fgSetMinMax(meta, pars->normalisation * pars->ref[i]);
    }

    // Complete meta data

    meta->duration  = pars->time[5];
    meta->range.end = config->final;

    // Check limits if supplied

    if(limits != NULL)
    {
        // Check limits at the end of the parabolic acceleration (segment 1)

        if((fg_error = fgCheckRef(limits, limits_polarity, pars->normalisation * pars->ref[1],
                                 (pars->normalisation * pars->acceleration * (pars->time[1] - pars->time[0])),
                                  pars->acceleration, meta)))
        {
            meta->error.index = 1;
            goto error;
        }

        // Check limits at the end of the linear segment (segment 2)

        if(pars->time[2] > pars->time[1] &&
           (fg_error = fgCheckRef(limits, limits_polarity, pars->normalisation * pars->ref[2],
                                  pars->normalisation * pars->linear_rate,
                                  pars->acceleration, meta)))
        {
            meta->error.index = 2;
            goto error;
        }

        // Check limits at the end of the exponential decay (segment 3)

        if((fg_error = fgCheckRef(limits, limits_polarity, pars->normalisation * pars->ref[3],
                                 (pars->normalisation * pars->acceleration * (pars->time[4] - pars->time[3])),
                                  pars->acceleration, meta)))
        {
            meta->error.index = 3;
            goto error;
        }

        // Check limits at the end of the parabolic deceleration (segment 4)

        if((fg_error = fgCheckRef(limits, limits_polarity, pars->normalisation * pars->ref[4],
                                  0.0, pars->acceleration, meta)))
        {
            meta->error.index = 4;
            goto error;
        }

        // Check limits at the end of the parabolic acceleration (segment 5)

        if((fg_error = fgCheckRef(limits, limits_polarity, pars->normalisation * pars->ref[5],
                                 config->final_rate, pars->acceleration, meta)))
        {
            meta->error.index = 5;
            goto error;
        }
    }

    return(FG_OK);

    // Error - store error code in meta and return to caller

    error:

        meta->fg_error = fg_error;
        return(fg_error);
}



bool fgPlepGen(struct fg_plep_pars *pars, const double *time, float *ref)
{
    bool     ref_running = true;            // Reference running flag is the return value
    float    r;                             // Normalised reference
    double   func_time;                     // Time within function
    float    seg_time;                      // Time within segment

    // Both *time and delay must be 64-bit doubles if time is UNIX time

    func_time = *time - pars->delay;

    // Pre-acceleration coast

    if(func_time <= 0.0)
    {
        r = pars->ref[0];
    }

    // Parabolic acceleration

    else if(func_time <= pars->time[1])
    {
        // seg_time is relative to start of accelerating parabola

        seg_time = func_time - pars->time[0];

        r = pars->ref[0] + 0.5 * pars->acceleration * seg_time * seg_time;
    }

    // Linear ramp

    else if(func_time <= pars->time[2])
    {
        // seg_time is relative to start of linear segment

        seg_time = func_time - pars->time[1];

        r = pars->ref[1] + pars->linear_rate * seg_time;
    }

    // Exponential deceleration

    else if(func_time <= pars->time[3])
    {
        // seg_time is relative to start of exponential segment

        seg_time = func_time - pars->time[2];

        r = pars->ref_exp * exp(pars->inv_exp_tc * seg_time) + pars->exp_final;
    }

    // Parabolic deceleration

    else if(func_time < pars->time[4])
    {
        // seg_time is relative to end of parabola (negative)

        seg_time = func_time - pars->time[4];

        r = pars->ref[4] - 0.5 * pars->acceleration * seg_time * seg_time;
    }

    // Parabolic acceleration

    else if(func_time < pars->time[5])
    {
        // seg_time is relative to start of parabola

        seg_time = func_time - pars->time[4];

        r = pars->ref[4] + 0.5 * pars->final_acc * seg_time * seg_time;
    }

    // Beyond end continue linear ramp using final_rate

    else
    {
        ref_running = false;

        // seg_time is relative to end of function

        seg_time = func_time - pars->time[5];

        r = pars->ref[5] + pars->final_rate * seg_time;
    }

    // De-normalise the result (reflect about zero for ascending PLEPs)

    *ref = pars->normalisation * r;

    return(ref_running);
}

// EOF

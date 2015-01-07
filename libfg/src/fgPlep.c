/*!
 * @file  fgPlep.c
 * @brief Generate Parabola-Linear-Exponential-Parabola (PLEP) function
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
#include "libfg/plep.h"



enum fg_error fgPlepInit(struct fg_limits *limits, 
                         bool   is_pol_switch_auto,
                         bool   is_pol_switch_neg,
                         double delay, 
                         float  initial_ref,
                         float  final_ref,
                         float  final_rate,
                         float  acceleration,
                         float  linear_rate,
                         float  exp_tc,
                         float  exp_final,
                         struct fg_plep *pars, 
                         struct fg_meta *meta)
{
    enum fg_error  fg_error;                      // Error code
    struct fg_meta local_meta;                    // Local meta data in case user meta is NULL
    struct fg_plep p;                             // Local PLEP pars - copied to user *pars only if there are no errors
    uint32_t       i;                             // Loop variable
    bool           is_exp;                        // Is exponential segment present flag
    bool           is_par_b4_lin;                 // Is decelerating parabola before linear flag
    bool           is_par_b4_exp;                 // Is decelerating parabola before exponential flag
    bool           is_exp_b4_lin;                 // Is exponential before linear flag
    float          delta_ref;                     // Initial ref minus final ref
    float          min_exp_final;                 // Minimum allowable exp_final
    float          exp_rate;                      // Initial rate of change for exponential segment
    float          delta_time1_par;               // Time between the top of first parabolic segment and the start of the second one
    float          delta_time1_lin;               // Time between the top of first parabolic segment and the start of linear segment
    float          delta_time1_exp;               // Time until exponential segment
    float          ref_time;                      // Segment time accumulator
    float          inv_acc;                       // 1 / p.acceleration
    float          delta_time[FG_PLEP_NUM_SEGS+1];// Segment durations

    meta = fgResetMeta(meta, &local_meta, delay, initial_ref);  // Reset meta structure - uses local_meta if meta is NULL

    // Check parameters are valid

    if(acceleration == 0.0 || linear_rate == 0.0 || (exp_tc > 0.0 &&
       (final_ref == 0.0 || final_rate != 0.0 || fabs(exp_final) >= fabs(final_ref))))
    {
        fg_error = FG_BAD_PARAMETER;
        goto error;
    }

    // Calculate PLEP parameters with zero initial rate of change

    // Prepare variables

    p.acceleration = fabs(acceleration);
    p.linear_rate  = fabs(linear_rate);
    p.final_rate   = final_rate;
    p.delay        = delay;
    p.ref_exp      = 0.0;
    p.inv_exp_tc   = 0.0;
    p.exp_final    = 0.0;

    delta_ref = initial_ref - final_ref;        // Total reference change
    inv_acc   = 1.0 / p.acceleration;           // Inverse acceleration

    delta_time[0] = delta_time[1] = delta_time[2] = delta_time[3] = 0.0; // Clear segment times

    // Calculate final accelerating parabola

    if(p.final_rate >= 0.0)
    {
        p.final_acc = p.acceleration;
    }
    else
    {
        p.final_acc = -p.acceleration;
    }

    delta_time[5] = p.final_rate / p.final_acc;
    p.ref[4]      = final_ref - 0.5 * p.final_rate * delta_time[5];

    // Allow exponential segment if all conditions are satisfied :

    is_exp = (p.final_rate == 0.0                &&     // The final rate is zero
             initial_ref * final_ref >= 0.0      &&     // The PLEP doesn't cross zero
             fabs(initial_ref) > fabs(final_ref) &&     // The magnitude of the current is decreasing
             exp_tc > 0.0);                             // The exponential time constant is greater than zero

    // Normalise if PLEP is ascending by reflecting across ref = 0

    if(delta_ref >= 0.0)                       // Descending PLEP
    {
        p.normalisation = 1.0;
        p.ref[5]        = final_ref;                // Normalised final reference
    }
    else                                       // Ascending PLEP
    {
        p.normalisation = -1.0;
        initial_ref     = -initial_ref;              // Normalised initial reference
        delta_ref       = -delta_ref;                // Normalised delta
        p.ref[4]        = -p.ref[4];                 // Normalised penultimate reference
        p.ref[5]        = -final_ref;                // Normalised final reference
        p.final_rate    = -p.final_rate;             // Normalised final rate
        p.final_acc     = -p.final_acc;              // Normalised final acceleration
    }

    p.ref[0] = initial_ref;

    // Prepare for exponential section if required

    if(is_exp)
    {
        p.inv_exp_tc = -1.0 / exp_tc;

        min_exp_final = final_ref - 0.5 * p.acceleration * exp_tc * exp_tc;

        // Clip exp final at minimum permitted value

        if(exp_final < min_exp_final)  
        {
            p.exp_final = min_exp_final;
        }
        else
        {
            p.exp_final = exp_final;
        }

        // Calculate rate at start of exp segment

        exp_rate = (p.exp_final - initial_ref) * p.inv_exp_tc;

        // Clip linear rate of change to exp limit

        if(exp_rate < p.linear_rate)
        {
            p.linear_rate = exp_rate;
        }
    }

    // Calculate PLEP parameters for different cases: 1.P-P, 2.P-E-P, 3.P-L-P, 4.P-L-E-P

    p.linear_rate   = -p.linear_rate;   // Always strictly negative
    p.acceleration  = -p.acceleration;  // Always strictly negative
    inv_acc         = -inv_acc;         // Always strictly negative

    // Delta times between the start of the first parabola and either the start of the second
    // parabola (P-P, P-E-P cases) or the start of the linear segment (P-L-P, P-L-E-P cases)

    delta_time1_par = sqrt(inv_acc * (p.ref[4] - p.ref[0]));
    delta_time1_lin = p.linear_rate * inv_acc;

    if(is_exp)
    {
        delta_time1_exp = sqrt(exp_tc * exp_tc +
                               2.0 * inv_acc * (p.exp_final - p.ref[0])) - exp_tc;
    }
    else
    {
        delta_time1_exp = 1.0E30;
    }

    is_par_b4_lin = (delta_time1_par < delta_time1_lin);
    is_par_b4_exp = (delta_time1_par < delta_time1_exp);
    is_exp_b4_lin = (delta_time1_exp < delta_time1_lin);

    // Case 1 : P-P

    if(is_par_b4_lin && is_par_b4_exp)
    {
        delta_time[1] = delta_time[4] = delta_time1_par;
        p.ref[1] = p.ref[2] = p.ref[3] = 0.5 * (p.ref[0] + p.ref[4]);

        goto end;
    }

    // Case 2 : P-E-P

    if(is_exp_b4_lin)
    {
        delta_time[1] = delta_time1_exp;
        p.ref[1] = p.ref[2] = p.ref[0] + 0.5 * p.acceleration * delta_time[1] * delta_time[1];

        exp_par:        // From goto exp_par;

        delta_time[4] = exp_tc - sqrt(exp_tc * exp_tc +
                                              2.0 * inv_acc * (p.ref[4] - p.exp_final));

        delta_time[3] = -exp_tc * log((p.ref[4] - p.exp_final -
                                               0.5 * p.acceleration * delta_time[4] * delta_time[4]) /
                                              (p.ref[2] - p.exp_final));

        p.ref[3]  = p.ref[4] - 0.5 * p.acceleration * delta_time[4] * delta_time[4];
        p.ref_exp = p.ref[2] - p.exp_final;

        goto end;
    }

    // Case 3, 4 : P-L-P or P-L-E-P

    delta_time[1] = delta_time1_lin;
    p.ref[1] = p.ref[0] + 0.5 * p.acceleration * delta_time[1] * delta_time[1];

    p.ref[3] = p.ref[4] - 0.5 * inv_acc * p.linear_rate * p.linear_rate;

    if(is_exp)
    {
        p.ref[2] = p.exp_final - p.linear_rate * exp_tc;

        if(p.ref[2] > p.ref[3])
        {
            // Case 4 : P-L-E-P

            delta_time[2] = (p.ref[2] - p.ref[1]) / p.linear_rate;

            goto exp_par;       // part of Case 2: P-E-P above
        }
    }

    // Case 3 : P-L-P

    p.ref[2]      = p.ref[3];
    delta_time[2] = (p.ref[2] - p.ref[1]) / p.linear_rate;
    delta_time[4] = p.linear_rate * inv_acc;

    // Calculate segment times and min/max reference 

    end:        // From goto end;

    ref_time = 0.0;

    for(i = 0 ; i <= FG_PLEP_NUM_SEGS ; i++)
    {
        ref_time += delta_time[i];
        p.time[i] = ref_time;

        fgSetMinMax(meta, p.normalisation * p.ref[i]);
    }

    // Complete meta data

    meta->duration  = p.time[5];
    meta->range.end = final_ref;

    fgSetFuncPolarity(meta, is_pol_switch_auto, is_pol_switch_neg);

    // Check limits if supplied

    if(limits != NULL)
    {
        // Check limits at the end of the parabolic acceleration (segment 1)

        if((fg_error = fgCheckRef(limits, p.normalisation * p.ref[1],
                                 (p.normalisation * p.acceleration * (p.time[1] - p.time[0])),
                                  p.acceleration, meta)))
        {
            meta->error.index = 1;
            goto error;
        }

        // Check limits at the end of the linear segment (segment 2)

        if(p.time[2] > p.time[1] &&
           (fg_error = fgCheckRef(limits, p.normalisation * p.ref[2],
                                  p.normalisation * p.linear_rate,
                                  p.acceleration, meta)))
        {
            meta->error.index = 2;
            goto error;
        }

        // Check limits at the end of the exponential decay (segment 3)

        if((fg_error = fgCheckRef(limits, p.normalisation * p.ref[3],
                                 (p.normalisation * p.acceleration * (p.time[4] - p.time[3])),
                                  p.acceleration, meta)))
        {
            meta->error.index = 3;
            goto error;
        }

        // Check limits at the end of the parabolic deceleration (segment 4)

        if((fg_error = fgCheckRef(limits, p.normalisation * p.ref[4],
                                  0.0, p.acceleration, meta)))
        {
            meta->error.index = 4;
            goto error;
        }

        // Check limits at the end of the parabolic acceleration (segment 5)

        if((fg_error = fgCheckRef(limits, p.normalisation * p.ref[5],
                                  final_rate, p.acceleration, meta)))
        {
            meta->error.index = 5;
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



enum fg_gen_status fgPlepGen(struct fg_plep *pars, const double *time, float *ref)
{
    enum fg_gen_status status = FG_GEN_DURING_FUNC; // Set default return status
    float              r;                           // Normalised reference
    double             func_time;                   // Time within function
    float              seg_time;                    // Time within segment

    // Both *time and delay must be 64-bit doubles if time is UNIX time

    func_time = *time - pars->delay;

    // Pre-acceleration coast

    if(func_time < 0.0)
    {
        r = pars->ref[0];

        status = FG_GEN_BEFORE_FUNC;
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
        status = FG_GEN_AFTER_FUNC;

        // seg_time is relative to end of function

        seg_time = func_time - pars->time[5];

        r = pars->ref[5] + pars->final_rate * seg_time;
    }

    // De-normalise the result (reflect about zero for ascending PLEPs)

    *ref = pars->normalisation * r;

    return(status);
}

// EOF

/*!
 * @file  test.c
 * @brief Generate test functions (STEPS, SQUARE, SINE or COSINE)
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

#include "libfg/test.h"



enum fg_error fgTestInit(struct fg_limits          *limits,
                         enum   fg_limits_polarity  limits_polarity,
                         struct fg_test_config     *config,
                         double                     delay,
                         float                      init_ref,
                         struct fg_test_pars       *pars,
                         struct fg_meta            *meta)
{
    enum fg_error  fg_error;       // Limits status
    uint32_t       n_cyc;          // int(num_cycles)
    float          inv_n_cyc;      // 1.0 / n_cyc
    float          end;            // Final reference value
    float          range[2];       // Range of reference value
    struct fg_meta local_meta;     // Local meta data in case user meta is NULL

    // Reset meta structure - uses local_meta if meta is NULL

    meta = fgResetMeta(meta, &local_meta, init_ref);

    // Prepare parameter structure

    n_cyc     = (uint32_t)(config->num_cycles + 0.4999);
    inv_n_cyc = 1.0 / (float)n_cyc;

    pars->duration    = (float)n_cyc * config->period;
    pars->half_period = 0.5 * config->period;
    pars->delay       = delay;
    pars->frequency   = 1.0 / config->period;
    pars->ref_amp     = config->amplitude_pp;
    pars->type        = config->type;
    pars->use_window  = config->use_window;

    // Check if total duration is too long

    if(pars->duration > 1.0E6)
    {
        meta->error.data[0] = pars->duration;
        meta->error.data[1] = 1.0E6;

        fg_error = FG_INVALID_TIME;
        goto error;
    }

    // Calculate amplitude related parameters

    end = pars->ref_initial = pars->ref_final = init_ref;

    fg_error = FG_OK;

    switch(config->type)
    {
        case FG_TEST_STEPS:

            pars->ref_final += pars->ref_amp;
            pars->ref_amp   *= inv_n_cyc;

            end      = pars->ref_final;
            range[0] = pars->ref_initial;
            range[1] = pars->ref_final;

            fgSetMinMax(meta, range[1]);

            // Check clip limits only if supplied

            if(limits != NULL)
            {
                if((fg_error = fgCheckRef(limits, limits_polarity, pars->ref_final, 0.0, 0.0, meta)))
                {
                    goto error;
                }
            }

            break;

        case FG_TEST_SQUARE:

            range[0] = pars->ref_initial;
            range[1] = pars->ref_initial + pars->ref_amp;

            fgSetMinMax(meta, range[1]);

            // Check clip limits only if supplied

            if(limits != NULL)
            {
                if((fg_error = fgCheckRef(limits, limits_polarity, pars->ref_initial + pars->ref_amp, 0.0, 0.0, meta)))
                {
                    goto error;
                }
            }

            break;

        case FG_TEST_SINE:
        case FG_TEST_COSINE:

            pars->ref_amp *= 0.5;
            range[0]       = pars->ref_initial - pars->ref_amp;
            range[1]       = pars->ref_initial + pars->ref_amp;

            fgSetMinMax(meta, range[1]);

            if(limits != NULL)  // Check clip limits only if supplied
            {
                if((fg_error = fgCheckRef(limits, limits_polarity, pars->ref_initial + pars->ref_amp, 0.0, 0.0, meta)) ||
                   (fg_error = fgCheckRef(limits, limits_polarity, pars->ref_initial - pars->ref_amp, 0.0, 0.0, meta)))
                {
                    goto error;
                }
            }

            break;

        default: // Invalid function type requested

            meta->error.data[0] = config->type;

            fg_error = FG_BAD_PARAMETER;
            goto error;
    }

    // Complete meta data

    meta->duration  = pars->duration;
    meta->range.end = end;

    return(FG_OK);

    // Error - store error code in meta and return to caller

    error:

        meta->fg_error = fg_error;
        return(fg_error);
}



bool fgTestGen(struct fg_test_pars *pars, const double *time, float *ref)
{
    uint32_t    period_idx;
    double      radians;
    float       cos_rads = 0.0;
    float       delta_ref;
    float       new_ref;
    double      func_time;                     // Time within function

    // Both *time and delay must be 64-bit doubles if time is UNIX time

    func_time = *time - pars->delay;

    // Pre-acceleration coast

    if(func_time <= 0.0)
    {
        *ref = pars->ref_initial;

        return(true);
    }

    // Operate N cycles following delay

    else if(func_time < pars->duration)
    {
        switch(pars->type)
        {
            case FG_TEST_STEPS:

                period_idx = 1 + (uint32_t)(func_time * pars->frequency);
                new_ref    = pars->ref_initial + pars->ref_amp * (float)period_idx;

                if(*ref != new_ref)
                {
                    *ref = new_ref;
                }

                return(true);

            case FG_TEST_SQUARE:

                period_idx = 1 - ((uint32_t)(2.0 * func_time * pars->frequency) & 0x1);
                new_ref    = pars->ref_initial + (period_idx ? pars->ref_amp : 0.0);

                if(*ref != new_ref)
                {
                    *ref = new_ref;
                }

                return(true);

            case FG_TEST_SINE:

                radians   = (2.0 * 3.1415926535897932) * pars->frequency * func_time;
                delta_ref = pars->ref_amp * sin(radians);
                break;

            case FG_TEST_COSINE:

                radians   = (2.0 * 3.1415926535897932) * pars->frequency * func_time;
                cos_rads  = cos(radians);
                delta_ref = pars->ref_amp * cos_rads;
                break;

            default: // Invalid function type requested

                return(false);
        }

        // For SINE and COSINE: Apply cosine window if enabled

        if(pars->use_window &&
          (func_time < pars->half_period ||
           pars->duration - func_time < pars->half_period))
        {
            // Calc Cosine window

           delta_ref *= 0.5 * (1 - (pars->type == FG_TEST_SINE ? cos(radians) : cos_rads));
        }

        *ref = pars->ref_initial + delta_ref;
    }

    // Coast

    else
    {
        *ref = pars->ref_final;

        return(false);
    }

    return(true);
}

// EOF

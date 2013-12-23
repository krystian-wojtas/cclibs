/*---------------------------------------------------------------------------------------------------------*\
  File:     test.c                                                                      Copyright CERN 2014

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

  Purpose:  Generate test functions (STEPS, SQUARE, SINE or COSINE)
\*---------------------------------------------------------------------------------------------------------*/

#include "libfg/test.h"

/*---------------------------------------------------------------------------------------------------------*/
enum fg_error fgTestInit(struct fg_limits        *limits,
                         enum fg_limits_polarity  limits_polarity,
                         struct fg_test_config   *config,
                         float                    delay,
                         float                    ref,
                         struct fg_test_pars     *pars,
                         struct fg_meta          *meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;       // Limits status
    uint32_t      n_cyc;          // int(num_cycles)
    uint32_t      negative_flag;  // Flag to indicate to limits check that part of reference is negative
    float         inv_n_cyc;      // 1.0 / n_cyc
    float         end;            // Final reference value
    float         range[2];       // Range of reference value

    fgResetMeta(meta);            // Reset meta structure

    // Prepare parameter structure

    n_cyc     = (uint32_t)(config->num_cycles + 0.4999);
    inv_n_cyc = 1.0 / (float)n_cyc;

    pars->duration    = (float)n_cyc * config->period;
    pars->half_period = 0.5 * config->period;
    pars->delay       = delay;
    pars->frequency   = 1.0 / config->period;
    pars->ref_amp     = config->amplitude_pp;
    pars->type        = config->type;
    pars->window_flag = config->window_flag;            // Allow windowing

    // Check parameters

    if(pars->duration > 1.0E5)                          // If total time is too long
    {
        if(meta != NULL)
        {
            meta->error.data[0]=pars->duration;
        }

        return(FG_INVALID_TIME);                                // Report INVALID TIME
    }

    // Calculate amplitude related parameters

    end = pars->ref_initial = pars->ref_final = ref;

    fg_error = FG_OK;

    switch(config->type)
    {
        case FG_TEST_STEPS:

            pars->ref_final += pars->ref_amp;                   // Calculate final ref value
            pars->ref_amp   *= inv_n_cyc;                       // Calculate step size

            end      = pars->ref_final;
            range[0] = pars->ref_initial;
            range[1] = pars->ref_final;

            if(limits)
            {
                negative_flag = range[0] < 0.0 || range[1] < 0.0;

                if((fg_error = fgCheckRef(limits, limits_polarity, negative_flag, pars->ref_final, 0.0, 0.0, meta)))
                {
                    return(fg_error);
                }
            }

            break;

        case FG_TEST_SQUARE:

            range[0] = pars->ref_initial;
            range[1] = pars->ref_initial + pars->ref_amp;

            if(limits)
            {
                negative_flag = range[0] < 0.0 || range[1] < 0.0;

                if((fg_error = fgCheckRef(limits, limits_polarity, negative_flag,
                                         pars->ref_initial + pars->ref_amp, 0, 0, meta)))
                {
                    return(fg_error);
                }
            }

            break;

        case FG_TEST_SINE:
        case FG_TEST_COSINE:

            pars->ref_amp *= 0.5;                      // Convert amplitude to 1/2 peak-peak
            range[0]       = pars->ref_initial - pars->ref_amp;
            range[1]       = pars->ref_initial + pars->ref_amp;

            if(limits)
            {
                negative_flag = range[0] < 0.0 || range[1] < 0.0;

                if((fg_error = fgCheckRef(limits, limits_polarity, negative_flag, pars->ref_initial + pars->ref_amp, 0.0, 0.0, meta)) ||
                   (fg_error = fgCheckRef(limits, limits_polarity, negative_flag, pars->ref_initial - pars->ref_amp, 0.0, 0.0, meta)))
                {
                    return(fg_error);
                }
            }

            break;

        default:                                            // Invalid function type requested

            if(meta != NULL)
            {
                meta->error.data[0] = config->type;
            }
            return(FG_BAD_PARAMETER);
    }

    pars->end_time = pars->duration + pars->delay;

// Return meta data

    if(meta != NULL)
    {
        meta->duration    = pars->end_time;
        meta->range.start = pars->ref_initial;
        meta->range.end   = end;

        if(range[0] < range[1])
        {
            meta->range.min = range[0];
            meta->range.max = range[1];
        }
        else
        {
            meta->range.min = range[1];
            meta->range.max = range[0];
        }
    }

    return(FG_OK);                      // Report success
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t fgTestGen(struct fg_test_pars *pars, const double *time, float *ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function derives the reference for a test function (STEPS, SQUARE, SINE or COSINE).
  It returns 1 while the function is in progress and 0 when it has been completed.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    period_idx;
    double      ref_time;
    double      radians;
    float       cos_rads = 0.0;
    float       delta_ref;
    float       new_ref;

    // Coast during run delay

    if(*time <= pars->delay)
    {
        *ref = pars->ref_initial;
        return(1);
    }

    // Operate N cycles following delay

    else if(*time < pars->end_time)
    {
        ref_time = *time - pars->delay;

        switch(pars->type)
        {
            case FG_TEST_STEPS:

                period_idx = 1 + (uint32_t)(ref_time * pars->frequency);
                new_ref    = pars->ref_initial + pars->ref_amp * (float)period_idx;

                if(*ref != new_ref) // This is an edge
                {
                    *ref = new_ref;
                }
                return(1);

            case FG_TEST_SQUARE:

                period_idx = 1 - ((uint32_t)(2.0 * ref_time * pars->frequency) & 0x1);
                new_ref    = pars->ref_initial + (period_idx ? pars->ref_amp : 0.0);

                if(*ref != new_ref) // This is an edge
                {
                    *ref = new_ref;
                }
                return(1);

            case FG_TEST_SINE:

                radians   = (2.0 * FG_PI) * pars->frequency * ref_time;
                delta_ref = pars->ref_amp * sin(radians);
                break;

            case FG_TEST_COSINE:

                radians   = (2.0 * FG_PI) * pars->frequency * ref_time;
                cos_rads  = cos(radians);
                delta_ref = pars->ref_amp * cos_rads;
                break;

            default: // Invalid function type requested

                return(0);
        }

        // Apply cosine window if enabled

        if(pars->window_flag &&                             // If window enabled, and
          (ref_time < pars->half_period ||                  // first or
           pars->duration - ref_time < pars->half_period))  // last half period
        {
           delta_ref *= 0.5 * (1 - (pars->type == FG_TEST_SINE ?      // Calc Cosine window
                                    cos(radians) : cos_rads));
        }

        *ref = pars->ref_initial + delta_ref;
    }

    // Coast

    else
    {
        *ref = pars->ref_final;         // Set ref to final value
        return(0);
    }

    return(1);
}
// EOF


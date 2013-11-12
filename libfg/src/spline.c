/*---------------------------------------------------------------------------------------------------------*\
  File:     spline.c                                                                    Copyright CERN 2011

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

  Purpose:  Generate spline interpolated table functions
\*---------------------------------------------------------------------------------------------------------*/

#include "libfg/spline.h"

/*---------------------------------------------------------------------------------------------------------*/
enum fg_error fgSplineInit(struct fg_limits         *limits,
                           enum   fg_limits_polarity limits_polarity,
                           struct fg_table_config   *config,
                           float                     delay,
                           float                     min_time_step,
                           struct fg_spline_pars    *pars,
                           struct fg_meta           *meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;       // Limit checking fg_error
    uint32_t      i;              // loop variable
    uint32_t      negative_flag;  // Flag to indicate to limits check that part of reference is negative
    float         spline_ref;     // Reference at spline connection point
    float         min;            // Minimim value
    float         max;            // Maximum value

    fgResetMeta(meta);            // Reset meta structure

    // Initial checks of data integrity

    if(config->ref_n_elements < 2 ||                            // If less than 2 points or
       config->ref_n_elements != config->time_n_elements)       // time and ref arrays are not the same length
    {
        return(FG_BAD_ARRAY_LEN);                                   // Report bad array len
    }

    if(config->time[0] != 0.0)                                  // If first time value is not zero
    {
        if(meta != NULL)
        {
            meta->error.data[0] = config->time[0];
        }
        return(FG_INVALID_TIME);                                    // Report invalid time
    }

    // Prepare spline parameters

    pars->acc_limit    = 0.0;                                   // Reset acceleration limit
    pars->delay        = delay;                                 // Time before start of function
    pars->n_elements   = config->ref_n_elements;                // Reference array length
    pars->ref          = config->ref;                           // Reference value array
    pars->time         = config->time;                          // Reference time array
    pars->seg_idx      = 0;                                     // Reset segment index
    pars->prev_seg_idx = 0;                                     // Reset previous segment index
    min = max          = config->ref[0];                        // Initialise min/max
    min_time_step     *= (1.0 - FG_CLIP_LIMIT_FACTOR);          // Adjust min time step to avoid rounding errs

    // Check time vector

    for(i = 1 ; i < pars->n_elements ; i++)
    {
        if(pars->time[i] < (pars->time[i - 1] + min_time_step))        // Check time values
        {
            if(meta != NULL)
            {
                meta->error.index     = i;
                meta->error.data[0] = pars->time[i];
                meta->error.data[1] = pars->time[i - 1] + min_time_step;
                meta->error.data[2] = min_time_step;
            }
            return(FG_INVALID_TIME);                                       // Report INVALID TIME
        }

        if(pars->ref[i] > max)
        {
            max = pars->ref[i];
        }

        if(pars->ref[i] < max)
        {
            min = pars->ref[i];
        }
    }

    // Check reference function limits

    // The complete time vector is checked above before checking the reference values as fgSplineCalc()
    // calculates the gradient of the following segment, which needs valid time values.

    if(limits != NULL)
    {
        pars->acc_limit = limits->acceleration;
        negative_flag   = min < 0.0 || max < 0.0;

        for(i = 1 ; i < pars->n_elements ; i++)
        {
            pars->seg_idx = i;

            fgSplineCalc(pars);

            spline_ref = pars->ref[i - 1] + pars->spline_time *
                        (0.5 * pars->acc_start * pars->spline_time + pars->grad_start);

            if((fg_error = fgCheckRef(limits, limits_polarity, negative_flag, spline_ref,
                                      pars->grad_spline, pars->acc_start, meta)) ||
               (fg_error = fgCheckRef(limits, limits_polarity, negative_flag, pars->ref[i],
                                      pars->grad_end, pars->acc_end, meta)))
            {
                if(meta != NULL)
                {
                    meta->error.index = i;
                }
                return(fg_error);
            }
        }
    }

// Return meta data

    if(meta != NULL)
    {
        meta->duration    = pars->time[i - 1] + pars->delay;
        meta->range.start = pars->ref[0];
        meta->range.end   = pars->ref[i - 1];
        meta->range.min   = min;
        meta->range.max   = max;
    }

    return(FG_OK);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t fgSplineGen(struct fg_spline_pars *pars, const double *time, float *ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function derives the reference if the spline function is selected.
\*---------------------------------------------------------------------------------------------------------*/
{
    double   ref_time;                                  // Time since end of run delay
    double   seg_time;                                  // Time within segment

    // Coast during run delay

    if(*time <= pars->delay)
    {
        *ref = pars->ref[0];                            // Set ref to inital value
        return(1);
    }

    // Check for end of segment

    ref_time = *time - pars->delay;

    while(ref_time >= pars->time[pars->seg_idx])      // while time exceeds end of segment
    {
        if(++pars->seg_idx >= pars->n_elements)                 // If vector complete
        {
            pars->seg_idx = pars->n_elements - 1;                   // Force segment index to last seg
            *ref          = pars->ref[pars->n_elements - 1];        // Enter coast
            return(0);
        }
    }

    while(ref_time < pars->time[pars->seg_idx - 1])      // while time before start of segment
    {
        pars->seg_idx--;
    }

    // If time is in a new segment, calculate the spline parameters for this segment

    if(pars->seg_idx != pars->prev_seg_idx)
    {
        pars->prev_seg_idx = pars->seg_idx;
        fgSplineCalc(pars);
    }

    // Calculate reference using spline parameters

    seg_time = ref_time - pars->time[pars->seg_idx - 1];

    if(seg_time < pars->spline_time)                  // If before spline connection
    {
        *ref        = pars->ref[pars->seg_idx - 1] +
                      seg_time * (0.5 * pars->acc_start * seg_time + pars->grad_start);
    }
    else                                        // else after spline connection
    {
        seg_time -= pars->seg_duration;                         // Time is negative (t=0 for end of segment)
        *ref        = pars->ref[pars->seg_idx] +
                      seg_time * (0.5 * pars->acc_end * seg_time + pars->grad_end);
    }

    return(1);
}
/*---------------------------------------------------------------------------------------------------------*/
void fgSplineCalc(struct fg_spline_pars *pars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will calculate the spline coefficients for the spline leading to the point pars->seg_idx.
\*---------------------------------------------------------------------------------------------------------*/
{
    float det;
    float spline_t_ratio;
    float grad_prev;
    float grad_seg;
    float grad_next;
    float delta_grad;
    float grad_seg_ratio;
    float min_grad_seg_ratio;
    float max_grad_seg_ratio;
    float normalised_acc_limit;
    float weight;               // Weight factor for gradient calculation

    // Calculate gradients for this segment and the segments before and after

    pars->seg_duration =  pars->time[pars->seg_idx] - pars->time[pars->seg_idx - 1];
    grad_seg           = (pars->ref [pars->seg_idx] - pars->ref [pars->seg_idx - 1]) / pars->seg_duration;

    if(pars->seg_idx == 1)
    {
        grad_prev = 0.0;
    }
    else
    {
        grad_prev = (pars->ref [pars->seg_idx - 1] - pars->ref [pars->seg_idx - 2]) /
                    (pars->time[pars->seg_idx - 1] - pars->time[pars->seg_idx - 2]);
    }

    if((pars->seg_idx + 1) >= pars->n_elements)
    {
        grad_next = 0.0;
    }
    else
    {
        grad_next = (pars->ref [pars->seg_idx + 1] - pars->ref [pars->seg_idx]) /
                    (pars->time[pars->seg_idx + 1] - pars->time[pars->seg_idx]);
    }

    // Calculate gradients for start and end of segment

    if(grad_seg == 0.0)                                 // If current segment is flat
    {
        pars->grad_start = 0.0;                                 // start and end gradients are zero
        pars->grad_end   = 0.0;
    }
    else                                                // else current segment is not flat
    {
        if(grad_prev == 0.0 ||                                  // If previous segment is flat, or
           grad_prev * grad_seg < 0.0)                          // gradient changes sign
        {
            pars->grad_start = 0.0;                                     // start gradient is zero
        }
        else                                                    // else gradients are same sign
        {
            weight = (float)(pars->time[pars->seg_idx - 1] - pars->time[pars->seg_idx - 2]) /
                     (float)(pars->time[pars->seg_idx]     - pars->time[pars->seg_idx - 2]);

            pars->grad_start = weight * (grad_seg - grad_prev) + grad_prev;
        }

        if(grad_next == 0.0 ||                                  // If next segment is flat, or
           grad_next * grad_seg < 0.0)                          // gradient changes sign
        {
            pars->grad_end = 0.0;                                     // end gradient is zero
        }
        else                                                    // else gradients are same sign
        {
             weight = (float)(pars->time[pars->seg_idx + 1] - pars->time[pars->seg_idx    ]) /
                      (float)(pars->time[pars->seg_idx + 1] - pars->time[pars->seg_idx - 1]);

             pars->grad_end = weight * (grad_seg - grad_next) + grad_next;
        }
    }

    // Calculate time and gradient of spline connection point

    delta_grad = pars->grad_end - pars->grad_start;     // Change in start and end gradients

    if(fabs(delta_grad) < 1.0E-10)                      // If start and end gradients are equal
    {
        spline_t_ratio    = 0.5;                                // Spline is at mid-point
        pars->grad_spline = 2.0 * grad_seg - pars->grad_end;    // Calculate spline connection gradient
    }
    else                                                // else start and end gradients are different
    {
        if(pars->acc_limit > 0.0)                       // If acceleration limit provided
        {
            normalised_acc_limit = fabs(pars->acc_limit * pars->seg_duration / delta_grad);

            min_grad_seg_ratio = 1.0 / (1.0 + normalised_acc_limit);
            max_grad_seg_ratio = normalised_acc_limit * min_grad_seg_ratio;
        }
        else
        {
            min_grad_seg_ratio = 0.001;
            max_grad_seg_ratio = 0.999;
        }

        grad_seg_ratio = (grad_seg - pars->grad_start) / delta_grad;

        // if segment ratio is in range to avoid exceeding acceleration limits

        if(min_grad_seg_ratio < grad_seg_ratio && grad_seg_ratio < max_grad_seg_ratio)
        {
            spline_t_ratio = 1.0 - grad_seg_ratio;              // Spline gradient will be segment gradient
        }
        else  // else minimise accelerations of each parabola by being equal and opposite
        {
            det = sqrt(0.5 + grad_seg_ratio * (grad_seg_ratio - 1.0));

            if(grad_seg_ratio < 0.5)
            {
                spline_t_ratio = 1.0 - grad_seg_ratio - det;
            }
            else if(grad_seg_ratio > 0.5)
            {
                spline_t_ratio = 1.0 - grad_seg_ratio + det;
            }
            else
            {
                spline_t_ratio = 0.5;                           // Spline is at mid-point
            }
        }

        pars->grad_spline = (spline_t_ratio + 2.0 * grad_seg_ratio - 1.0) * delta_grad + pars->grad_start;
    }

    pars->spline_time = spline_t_ratio * pars->seg_duration;

    // Calculate start and end accelerations

    pars->acc_start = (pars->grad_spline - pars->grad_start)  / pars->spline_time;
    pars->acc_end   = (pars->grad_end    - pars->grad_spline) / (pars->seg_duration - pars->spline_time);
}
/*---------------------------------------------------------------------------------------------------------*\
  End of file: spline.c
\*---------------------------------------------------------------------------------------------------------*/

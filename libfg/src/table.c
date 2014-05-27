/*---------------------------------------------------------------------------------------------------------*\
  File:     table.c                                                                     Copyright CERN 2014

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

  Purpose:  Generate linearly interpolated table functions
\*---------------------------------------------------------------------------------------------------------*/

#include "string.h"
#include "libfg/table.h"

/*---------------------------------------------------------------------------------------------------------*/
enum fg_error fgTableInit(struct fg_limits         *limits,
                          enum   fg_limits_polarity limits_polarity,
                          struct fg_table_config   *config,
                          float                     delay,
                          float                     min_time_step,
                          struct fg_table_pars     *pars,
                          struct fg_meta           *meta)          // NULL if not required
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error  fg_error;       // Limit checking status
    uint32_t       i;              // loop variable
    float          grad;           // Segment gradient
    struct fg_meta local_meta;     // Local meta data in case user meta is NULL

    meta = fgResetMeta(meta, &local_meta, config->ref[0]);  // Reset meta structure - uses local_meta if meta is NULL

    // Initial checks of data integrity

    if(config->ref_n_elements < 2 ||                        // If less than 2 points or
       config->ref_n_elements != config->time_n_elements)   // time and ref arrays are not the same length
    {
        return(FG_BAD_ARRAY_LEN);                               // Report bad array len
    }

    if(config->time[0] != 0.0)                              // If first time value is not zero
    {
        if(meta != NULL)
        {
            meta->error.data[0] = config->time[0];
        }
        return(FG_INVALID_TIME);                                // Report invalid time
    }

    // Prepare table parameters

    pars->delay        = delay;                             // Run delay
    pars->n_elements   = config->ref_n_elements;            // Reference array length
    pars->time         = config->time;                      // Reference time array
    pars->seg_idx      = 0;                                 // Reset segment index
    pars->prev_seg_idx = 0;                                 // Reset previous segment index
    min_time_step     *= (1.0 - FG_CLIP_LIMIT_FACTOR);      // Adjust min time step to avoid rounding errs

    if(pars->ref == NULL)
    {
        pars->ref = config->ref;                            // Reference value array
    }

    if(pars->time == NULL)
    {
        pars->time = config->time;                          // Reference time array
    }

    // Check time vector and calculate min/max for table

    for(i = 1 ; i < pars->n_elements ; i++)
    {
        if(pars->time[i] < (pars->time[i - 1] + min_time_step))        // Check time values
        {
            meta->error.index     = i;
            meta->error.data[0] = pars->time[i];
            meta->error.data[1] = pars->time[i - 1] + min_time_step;
            meta->error.data[2] = min_time_step;

            return(FG_INVALID_TIME);                                // Report INVALID TIME
        }

        fgSetMinMax(meta, pars->ref[i]);
    }

    // Check reference function limits

    if(limits != NULL)
    {
        for(i = 1 ; i < pars->n_elements ; i++)
        {
            grad = (pars->ref[i] - pars->ref[i - 1]) / (pars->time[i] - pars->time[i - 1]);

            if((fg_error = fgCheckRef(limits, limits_polarity, pars->ref[i],     grad, 0.0, meta)) ||
               (fg_error = fgCheckRef(limits, limits_polarity, pars->ref[i - 1], grad, 0.0, meta)))
            {
                meta->error.index = i;
                return(fg_error);
            }
        }
    }

    // Copy data if pars arrays are different to config arrays

    if(pars->ref != config->ref)
    {
        memcpy(pars->ref, config->ref, pars->n_elements * sizeof(pars->ref[0]));
    }

    if(pars->time != config->time)
    {
        memcpy(pars->time, config->time, pars->n_elements * sizeof(pars->time[0]));
    }

    // Complete meta data

    meta->duration  = pars->time[i - 1] + pars->delay;
    meta->range.end = pars->ref[i - 1];

    return(FG_OK);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t fgTableGen(struct fg_table_pars *pars, const double *time, float *ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function derives the reference for table functions.
\*---------------------------------------------------------------------------------------------------------*/
{
    double   ref_time;                               // Time since end of run delay

    // Coast during run delay

    if(*time <= pars->delay)
    {
        *ref = pars->ref[0];                                    // Set ref to initial value
        return(1);
    }

    // Scan through table to find segment containing the current time

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

    // If time is in a new segment, calculate the gradient

    if(pars->seg_idx != pars->prev_seg_idx)
    {
        pars->prev_seg_idx = pars->seg_idx;
        pars->seg_grad     = (pars->ref [pars->seg_idx] - pars->ref [pars->seg_idx - 1]) /
                             (pars->time[pars->seg_idx] - pars->time[pars->seg_idx - 1]);
    }

    // Calculate reference using segment gradient

    *ref = pars->ref[pars->seg_idx]  - (pars->time[pars->seg_idx] - ref_time) * pars->seg_grad;

    return(1);
}
// EOF


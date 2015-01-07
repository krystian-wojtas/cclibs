/*!
 * @file  fgTable.c
 * @brief Generate linearly interpolated table functions.
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

#include "string.h"
#include "libfg/table.h"



enum fg_error fgTableInit(struct   fg_limits *limits, 
                          bool     is_pol_switch_auto,
                          bool     is_pol_switch_neg,
                          double   delay, 
                          float    min_time_step,
                          float   *ref,
                          uint32_t ref_num_els,
                          float   *time,
                          uint32_t time_num_els,
                          struct   fg_table *pars, 
                          struct   fg_meta *meta)
{
    enum fg_error  fg_error;       // Limit checking status
    uint32_t       i;              // loop variable
    uint32_t       num_points;     // Number of points in the table
    float          grad;           // Segment gradient
    struct fg_meta local_meta;     // Local meta data in case user meta is NULL

    // Reset meta structure - uses local_meta if meta is NULL

    meta = fgResetMeta(meta, &local_meta, delay, ref[0]);

    // Initial checks of data integrity

    if(ref_num_els < 2 ||                          // If less than 2 points or
       ref_num_els != time_num_els)   // time and ref arrays are not the same length
    {
        meta->error.data[0] = (float)ref_num_els;
        meta->error.data[1] = (float)time_num_els;

        fg_error = FG_BAD_ARRAY_LEN;                            // Report bad array len
        goto error;
    }

    if(time[0] != 0.0)                              // If first time value is not zero
    {
        meta->error.data[0] = time[0];

        fg_error = FG_INVALID_TIME;                             // Report invalid time
        goto error;
    }

    // Check time vector and calculate min/max for table

    num_points     = ref_num_els;
    min_time_step *= (1.0 - FG_CLIP_LIMIT_FACTOR);      // Adjust min time step to avoid rounding errs

    for(i = 1 ; i < num_points ; i++)
    {
        if(time[i] < (time[i - 1] + min_time_step))        // Check time values
        {
            meta->error.index     = i;
            meta->error.data[0] = time[i];
            meta->error.data[1] = time[i - 1] + min_time_step;
            meta->error.data[2] = min_time_step;

            fg_error = FG_INVALID_TIME;                             // Report invalid time
            goto error;
        }

        fgSetMinMax(meta, ref[i]);
    }

    // Complete meta data

    meta->duration  = time[i - 1];
    meta->range.end = ref [i - 1];

    fgSetFuncPolarity(meta, is_pol_switch_auto, is_pol_switch_neg);

    // Check reference function limits if provided

    if(limits != NULL)
    {
        for(i = 1 ; i < num_points ; i++)
        {
            grad = (ref[i] - ref[i - 1]) / (time[i] - time[i - 1]);

            if((fg_error = fgCheckRef(limits, ref[i],     grad, 0.0, meta)) ||
               (fg_error = fgCheckRef(limits, ref[i - 1], grad, 0.0, meta)))
            {
                meta->error.index = i;
                goto error;
            }
        }
    }

    // Prepare table parameters

    pars->delay        = delay;
    pars->num_points   = num_points;
    pars->seg_idx      = 0;
    pars->prev_seg_idx = 0;

    if(pars->ref == NULL)
    {
        pars->ref = ref;
    }

    if(pars->time == NULL)
    {
        pars->time = time;
    }

    // Copy data if pars arrays are different

    if(pars->ref != ref)
    {
        memcpy(pars->ref, ref, num_points * sizeof(ref[0]));
    }

    if(pars->time != time)
    {
        memcpy(pars->time, time, num_points * sizeof(time[0]));
    }

    return(FG_OK);

    // Error - store error code in meta and return to caller

    error:

        meta->fg_error = fg_error;
        return(fg_error);
}



enum fg_gen_status fgTableGen(struct fg_table *pars, const double *time, float *ref)
{
    double   func_time;                     // Time within function

    // Both *time and delay must be 64-bit doubles if time is UNIX time

    func_time = *time - pars->delay;

    // Pre-acceleration coast

    if(func_time < 0.0)
    {
         *ref = pars->ref[0];

         return(FG_GEN_BEFORE_FUNC);
    }

    // Scan through table to find segment containing the current time

    while(func_time >= pars->time[pars->seg_idx])      // while time exceeds end of segment
    {
        if(++pars->seg_idx >= pars->num_points)                 // If vector complete
        {
            pars->seg_idx = pars->num_points - 1;                   // Force segment index to last seg
            *ref          = pars->ref[pars->num_points - 1];        // Enter coast

            return(FG_GEN_AFTER_FUNC);
        }
    }

    while(func_time < pars->time[pars->seg_idx - 1])      // while time before start of segment
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

    *ref = pars->ref[pars->seg_idx]  - (pars->time[pars->seg_idx] - func_time) * pars->seg_grad;

    return(FG_GEN_DURING_FUNC);
}

// EOF

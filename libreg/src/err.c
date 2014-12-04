/*!
 * @file  err.c
 * @brief Converter Control Regulation library regulation error functions
 *
 * <h2>Copyright</h2>
 *
 * Copyright CERN 2014. This project is released under the GNU Lesser General
 * Public License version 3.
 * 
 * <h2>License</h2>
 *
 * This file is part of libreg.
 *
 * libreg is free software: you can redistribute it and/or modify it under the
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

#include <math.h>
#include "libreg/err.h"



// Non-Real-Time Functions - do not call these from the real-time thread or interrupt

void regErrInitLimits(struct reg_err *err, float warning_threshold, float fault_threshold)
{
    // Set the new fault and warning limits

    err->warning.threshold = warning_threshold;
    err->fault.threshold   = fault_threshold;

    // If the fault or warning is disabled (set to zero), then reset the corresponding flag and counter.

    if(err->warning.threshold == 0.0)
    {
        err->warning.flag   = false;
        err->warning.filter = 0.0;
    }

    if(err->fault.threshold == 0.0)
    {
        err->fault.flag   = false;
        err->fault.filter = 0.0;
    }
}



// Real-Time Functions

void regErrResetLimitsVarsRT(struct reg_err *err, uint32_t inhibit_max_abs_err_counter)
{
    err->err            = 0.0;
    err->max_abs_err    = 0.0;

    err->warning.flag   = false;
    err->warning.filter = 0.0;

    err->fault.flag     = false;
    err->fault.filter   = 0.0;

    err->inhibit_max_abs_err_counter = inhibit_max_abs_err_counter;
}


/*!     
 * Manage the warning and fault limits by applying hysteresis to a first-order filter of the limit exceeded flag.
 *
 * @param[in,out] err_limit    Pointer to regulation error limit threshold and flags structure
 * @param[in]     abs_err      Absolute error
 */
static void regErrLimitRT(struct reg_err_limit *err_limit, float abs_err)
{
    float threshold = err_limit->threshold;

    // Apply hysteresis to the limit by dividing by 2 when the flag is set

    if(err_limit->flag)
    {
        threshold *= 0.5;
    }

    // Use first-order filter on the threshold exceeded flag
    
    err_limit->filter *= 0.9;

    if(abs_err > threshold)
    {
        err_limit->filter += 0.1;
    }

    // Set flag if the filtered threshold exceeded flag is more than 30%

    err_limit->flag = err_limit->filter > 0.3;
}



void regErrCheckLimitsRT(struct reg_err *err, float delayed_ref, float meas)
{
    float       abs_error;

    // Store delayed_ref so it can be logged if required

    err->delayed_ref = delayed_ref;

    // Calculate regulation error

    err->err = delayed_ref - meas;

    abs_error = fabs(err->err);

    // Down count the max_abs_err inhibit counter

    if(err->inhibit_max_abs_err_counter > 0)
    {
        // When down counter hits zero, reset max_abs_err to the current abs error value

        if(--err->inhibit_max_abs_err_counter == 0)
        {
            err->max_abs_err = abs_error;
        }
    }
    else if(abs_error > err->max_abs_err)
    {
        err->max_abs_err = abs_error;
    }

    // Check error warning and fault thresholds only if the threshold level is non-zero

    if(err->warning.threshold > 0.0)
    {
        regErrLimitRT(&err->warning, abs_error);
    }

    if(err->fault.threshold > 0.0)
    {
        regErrLimitRT(&err->fault, abs_error);
    }
}

// EOF

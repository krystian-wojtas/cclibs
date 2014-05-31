/*---------------------------------------------------------------------------------------------------------*\
  File:     err.c                                                                       Copyright CERN 2014

  License:  This file is part of libreg.

            libreg is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  Regulation error functions

            These functions can be used for any sort of regulation (current, field, voltage).  They
            maintain a history of the reference so that the measurement can be compared against the
            reference taking into account the tracking delay.
\*---------------------------------------------------------------------------------------------------------*/

#include <math.h>
#include "libreg/err.h"

//-----------------------------------------------------------------------------------------------------------
// Non-Real-Time Functions - do not call these from the real-time thread or interrupt
//-----------------------------------------------------------------------------------------------------------
void regErrInitLimits(struct reg_err *err, float warning_threshold, float fault_threshold)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to initialise the warning and fault limits of the reg_err structure.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Set the new fault and warning limits

    err->warning.threshold = warning_threshold;
    err->fault.threshold   = fault_threshold;

    // If the fault or warning is disabled (set to zero), then reset the corresponding flag and counter.

    if(err->warning.threshold == 0.0)
    {
        err->warning.flag   = 0;
        err->warning.filter = 0.0;
    }

    if(err->fault.threshold == 0.0)
    {
        err->fault.flag   = 0;
        err->fault.filter = 0.0;
    }
}
//-----------------------------------------------------------------------------------------------------------
// Real-Time Functions
//-----------------------------------------------------------------------------------------------------------
void regErrResetLimitsVarsRT(struct reg_err *err)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to reset the reg_err structure variables, except for the max_abs_err.
\*---------------------------------------------------------------------------------------------------------*/
{
    err->err            = 0.0;

    err->warning.flag   = 0;
    err->warning.filter = 0.0;

    err->fault.flag     = 0;
    err->fault.filter   = 0.0;
}
/*---------------------------------------------------------------------------------------------------------*/
static void regErrLimitRT(struct reg_err_limit *err_limit, float abs_err)
/*---------------------------------------------------------------------------------------------------------*\
  This function manages the warning and fault limits by applying hysteresis to a first order filter of the
  limit exceeded flag.
\*---------------------------------------------------------------------------------------------------------*/
{
    float threshold = err_limit->threshold;

    // Apply hysteresis to the limit by dividing by 2 when the flag is set

    if(err_limit->flag)
    {
        threshold *= 0.5;
    }

    // Use first order filter on the threshold exceeded flag
    
    err_limit->filter *= 0.9;

    if(abs_err > threshold)
    {
        err_limit->filter += 0.1;
    }

    // Set flag if the filtered threshold exceeded flag is more than 30%

    err_limit->flag = err_limit->filter > 0.3;
}
/*---------------------------------------------------------------------------------------------------------*/
void regErrCheckLimitsRT(struct reg_err *err, uint32_t enable_err, uint32_t enable_max_abs_err,
                         float delayed_ref, float meas)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to calculate the regulation error and to check the error limits (if supplied).
  The calculation of the error can be enabled by setting enable_err.
  The calculation of the max_abs_err can be enabled by setting enable_max_abs_err to be non-zero,
  otherwise max_abs_err is zeroed.
\*---------------------------------------------------------------------------------------------------------*/
{
    float       abs_error;

    // Store delayed_ref so it can be logged if required

    err->delayed_ref = delayed_ref;

    // Suppress error calculation if enable_err is zero

    if(enable_err == 0)
    {
        regErrResetLimitsVarsRT(err);
        return;
    }

    // Calculate regulation error

    err->err = delayed_ref - meas;

    abs_error = fabs(err->err);

    // Calculate or reset max abs err

    if(enable_max_abs_err != 0)
    {
        if(abs_error > err->max_abs_err)
        {
            err->max_abs_err = abs_error;
        }
    }
    else
    {
        err->max_abs_err = 0.0;
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


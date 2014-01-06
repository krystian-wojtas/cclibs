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
//#include "libreg/lim.h"

/*---------------------------------------------------------------------------------------------------------*/
void regErrInitDelay(struct reg_err *err, float *buf, float track_delay, float iter_period)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to initialise the reg_err delay structure. The buf pointer should be to a
  float array of length 1+(int)delay_in_iters.  If the function is called again then buf can be NULL and the
  old pointer to buf will be preserved. Similarly, if iter_period and track_delay are zero, the old 
  value will be preserved.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Save track_delay and iter_period if set, otherwise keep old value

    if(track_delay > 0.0)
    {
        err->track_delay = track_delay;
    }

    if(iter_period > 0.0)
    {
        err->iter_period = iter_period;
    }

    // Initialise (or re-initialise) delay parameters

    regDelayInitPars(&err->delay, buf, 1.0 + (err->track_delay / err->iter_period), 0);

    // Reset error limits and delay variables

    regDelayInitVars(&err->delay, 0.0);

    regErrResetLimitsVars(&err->limits);
}
/*---------------------------------------------------------------------------------------------------------*/
float regErrCalc(struct reg_err *err, uint32_t enable_err, uint32_t enable_max_abs_err,
                 float ref, float meas)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to calculate the regulation error and to check the error thresholds
  (if supplied).  The calculation isn't started until the delay history buffer is full (since the last time
  regErrInitDelay() was called).  The calculation of the max_abs_err can be enabled by setting
  enable_max_abs_err to be non-zero, otherwise max_abs_err is zeroed.
\*---------------------------------------------------------------------------------------------------------*/
{
    float   delayed_ref;

    // If reg_err structure not initialised then return immediately

    if(err->delay.buf == 0)
    {
        return(0.0);
    }

    // Calculate delayed reference and return if it's not yet valid or err calculation is not enabled

    if(regDelayCalc(&err->delay, ref, &delayed_ref) == 0)
    {
        enable_err = 0;
    }

    return(regErrCheckLimits(&err->limits, enable_err, enable_max_abs_err, delayed_ref, meas));
}
/*---------------------------------------------------------------------------------------------------------*/
void regErrInitLimits(struct reg_err_limits *err_limits,
                      float err_warning_threshold,
                      float err_fault_threshold)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to initialise the warning and fault limits of the reg_err_limits structure.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Set the new fault and warning limits

    err_limits->warning.threshold = err_warning_threshold;
    err_limits->fault.threshold   = err_fault_threshold;

    // If the fault or warning is disabled (set to zero), then reset the corresponding flag and counter.

    if(err_limits->warning.threshold == 0.0)
    {
        err_limits->warning.flag   = 0;
        err_limits->warning.filter = 0.0;
    }

    if(err_limits->fault.threshold == 0.0)
    {
        err_limits->fault.flag   = 0;
        err_limits->fault.filter = 0.0;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void regErrResetLimitsVars(struct reg_err_limits *err_limits)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to reset the reg_err_limits structure variables.
\*---------------------------------------------------------------------------------------------------------*/
{
    err_limits->err            = 0.0;
    err_limits->max_abs_err    = 0.0;

    err_limits->warning.filter = 0.0;
    err_limits->warning.flag   = 0;

    err_limits->fault.filter   = 0.0;
    err_limits->fault.flag     = 0;
}
/*---------------------------------------------------------------------------------------------------------*/
static void regErrLimit(struct reg_err_limit *err_limit, float abs_err)
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
float regErrCheckLimits(struct reg_err_limits *err_limits,
                        uint32_t enable_err,
                        uint32_t enable_max_abs_err,
                        float    delayed_ref,
                        float    meas)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to calculate the regulation error and to check the error limits (if supplied).
  The calculation of the max_abs_err can be enabled by setting enable_max_abs_err to be non-zero,
  otherwise max_abs_err is zeroed.
\*---------------------------------------------------------------------------------------------------------*/
{
    float       abs_error;

    // Store delayed_ref so it can be logged if required

    err_limits->delayed_ref = delayed_ref;

    // If err check is not enabled then reset limit variables

    if(enable_err == 0)
    {
        // Cannot call regErrResetLimitVars because it also zeros max_abs_err

        err_limits->err            = 0.0;

        err_limits->warning.filter = 0.0;
        err_limits->warning.flag   = 0;

        err_limits->fault.filter   = 0.0;
        err_limits->fault.flag     = 0;

        return(0.0);
    }

    // Calculate regulation error

    err_limits->err = delayed_ref - meas;

    abs_error = fabs(err_limits->err);

    // Calculate or reset max abs err

    if(enable_max_abs_err != 0)
    {
        if(abs_error > err_limits->max_abs_err)
        {
            err_limits->max_abs_err = abs_error;
        }
    }
    else
    {
        err_limits->max_abs_err = 0.0;
    }

    // Check error warning and fault thresholds only if the threshold level is non-zero

    if(err_limits->warning.threshold > 0.0)
    {
        regErrLimit(&err_limits->warning, abs_error);
    }

    if(err_limits->fault.threshold > 0.0)
    {
        regErrLimit(&err_limits->fault, abs_error);
    }

    return(err_limits->err);
}
// EOF


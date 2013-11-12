/*---------------------------------------------------------------------------------------------------------*\
  File:     err.c                                                                       Copyright CERN 2011

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
#include "libreg/lim.h"

/*---------------------------------------------------------------------------------------------------------*/
void regErrInitLimits(struct reg_err *err, float err_warning_limit, float err_fault_limit)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to initialise the warning and fault limits of the reg_err structure.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Set the new fault and warning limits

    err->warning_limit = err_warning_limit;
    err->fault_limit   = err_fault_limit;

    // If the fault or warning is disabled (set to zero), then reset the corresponding flag and counter.

    if(err->warning_limit == 0.0)
    {
        err->flags.warning  = 0;
        err->warning_filter = 0.0;
    }

    if(err->fault_limit == 0.0)
    {
        err->flags.fault  = 0;
        err->fault_filter = 0.0;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void regErrInitDelay(struct reg_err *err, float *buf, float track_delay, float iter_period)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to initialise the reg_err delay structure. The buf pointer should be to a
  float array of length 1+(int)delay_in_iters.  If the function is called again then buf can be NULL and the
  old pointer to buf will be preserved.
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

    // Keep filter period and re-initialise counter limit and reset error variables

    regErrInitVars(err);
}
/*---------------------------------------------------------------------------------------------------------*/
void regErrInitVars(struct reg_err *err)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to initialise the reg_err structure variables.
\*---------------------------------------------------------------------------------------------------------*/
{
    regDelayInitVars(&err->delay, 0.0);

    err->err              = 0.0;
    err->max_abs_err      = 0.0;
    err->fault_filter     = 0.0;
    err->warning_filter   = 0.0;
    err->flags.warning    = 0;
    err->flags.fault      = 0;
}
/*---------------------------------------------------------------------------------------------------------*/
static void regErrLimit(float abs_err, float limit, float *filter, uint32_t *flag)
/*---------------------------------------------------------------------------------------------------------*\
  This function manages the limits by applying hysteresis to a first order filter of the limit exceeded
  flag.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Apply hysteresis to the limit by dividing by 2 when the flag is set

    if(*flag)
    {
        limit *= 0.5;
    }

    // Use first order filter on limit_exceeded flag
    
    *filter *= 0.9;

    if(abs_err > limit)
    {
        *filter += 0.1;
    }

    // Set flag if the filtered limit flag is more than 30%

    *flag = *filter > 0.3;
}
/*---------------------------------------------------------------------------------------------------------*/
float regErrCalc(struct reg_err *err, uint32_t enable_err, uint32_t enable_max_abs_err,
                 float ref, float meas)
/*---------------------------------------------------------------------------------------------------------*\
  This function can be called to calculate the regulation error and to check the error limits (if supplied).
  The calculation isn't started until the history buffer is full (since the last time regErrInitVars() was
  called).  The calculation of the max_abs_err can be enabled by setting enable_max_abs_err to be non-zero,
  otherwise max_abs_err is zeroed.
\*---------------------------------------------------------------------------------------------------------*/
{
    float       abs_error;

    // If reg_err structure not initialised then return immediately

    if(err->delay.buf == 0)
    {
        return(0.0);
    }

    // Calculate delayed reference and return if it's not yet valid or err calculation is not enab

    if(regDelayCalc(&err->delay, ref, &err->delayed_ref) == 0 || enable_err == 0)
    {
        err->err = err->warning_filter = err->fault_filter = 0.0;
        err->flags.warning = err->flags.fault  = 0;
        return(0.0);
    }

    // Calculate regulation error

    err->err = err->delayed_ref - meas;

    abs_error = fabs(err->err);

    // Calculate or reset max abs err

    if(enable_max_abs_err)
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

    // Check error warning and fault limits only if the limit level is non-zero

    if(err->warning_limit > 0.0)
    {
        regErrLimit(abs_error, err->warning_limit, &err->warning_filter, &err->flags.warning);
    }

    if(err->fault_limit > 0.0)
    {
        regErrLimit(abs_error, err->fault_limit, &err->fault_filter, &err->flags.fault);
    }

    return(err->err);
}
/*---------------------------------------------------------------------------------------------------------*\
  End of file: err.c
\*---------------------------------------------------------------------------------------------------------*/

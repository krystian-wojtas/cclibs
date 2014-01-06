/*---------------------------------------------------------------------------------------------------------*\
  File:     libreg/err.h                                                                Copyright CERN 2014

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

  Purpose:  Converter Control Regulation library regulation error functions header file

\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBREG_ERR_H
#define LIBREG_ERR_H

#include <stdint.h>
#include <libreg/delay.h>

// Regulation error structures

struct reg_err_limit
{
    float                       threshold;                      // Limit threshold
    float                       filter;                         // Threshold exceeded flag filter
    uint32_t                    flag;                           // Limit exceeded flag
};


struct reg_err_limits
{
    float                       err;                            // Regulation error (reference-measurement)
    float                       max_abs_err;                    // Max absolute error
    struct reg_err_limit        warning;                        // Warning limit structure
    struct reg_err_limit        fault;                          // Fault limit structure
};

struct reg_err
{
    float                       track_delay;                    // Regulation tracking delay
    float                       iter_period;                    // Regulation iteration period
    float                       delayed_ref;                    // Delayed reference

    struct reg_delay            delay;                          // Signal delay structure
    struct reg_err_limits       limits;                         // Regulation error limits
};

#ifdef __cplusplus
extern "C" {
#endif

// Regulation error functions

void     regErrInitDelay        (struct reg_err *err, float *buf, float track_delay, float iter_period);
float    regErrCalc             (struct reg_err *err, uint32_t enable_err, uint32_t enable_max_abs_err, float ref, float meas);

void     regErrInitLimits       (struct reg_err_limits *err_limits, float err_warning_threshold,
                                 float err_fault_threshold);
void     regErrResetLimitsVars  (struct reg_err_limits *err_limits);
float    regErrCheckLimits      (struct reg_err_limits *err_limits, uint32_t enable_err,
                                 uint32_t enable_max_abs_err, float err);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_ERR_H

// EOF

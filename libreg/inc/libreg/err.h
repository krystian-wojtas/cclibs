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

// Regulation error structure (Error between unclipped reference and measurement)

struct reg_err
{
    struct reg_delay            delay;                          // Signal delay structure
    float                       track_delay;                    // Regulation tracking delay
    float                       iter_period;                    // Regulation iteration period
    float                       delayed_ref;                    // Delayed reference
    float                       err;                            // Regulation error
    float                       max_abs_err;                    // Max absolute error
    float                       warning_limit;                  // Error warning limit
    float                       fault_limit;                    // Error fault limit
    float                       warning_filter;                 // Warning limit exceeded flag filter
    float                       fault_filter;                   // Fault limit exceeded flag filter

    struct
    {
        uint32_t                warning;                        // Warning level flag
        uint32_t                fault;                          // Fault level flag
    } flags;
};

// Signal delay functions

#ifdef __cplusplus
extern "C" {
#endif

// Regulation error functions

void     regErrInitLimits       (struct reg_err *err, float err_warning_limit, float err_fault_limit);
void     regErrInitDelay        (struct reg_err *err, float *buf, float track_delay, float iter_period);
void     regErrInitVars         (struct reg_err *err);
float    regErrCalc             (struct reg_err *err, uint32_t enable_err, uint32_t enable_max_abs_err, float ref, float meas);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_ERR_H

// EOF

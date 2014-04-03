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

// Regulation error structures

struct reg_err_limit
{
    float                       threshold;                      // Limit threshold
    float                       filter;                         // Threshold exceeded flag filter
    uint32_t                    flag;                           // Limit exceeded flag
};


struct reg_err
{
    float                       delayed_ref;                    // Delayed reference
    float                       err;                            // Regulation error = delayed_ref - meas
    float                       max_abs_err;                    // Max absolute error
    struct reg_err_limit        warning;                        // Warning limit structure
    struct reg_err_limit        fault;                          // Fault limit structure
};

#ifdef __cplusplus
extern "C" {
#endif

// Regulation error functions

void     regErrInitLimits       (struct reg_err *err, float warning_threshold, float fault_threshold);
void     regErrResetLimitsVars  (struct reg_err *err);
float    regErrCheckLimits      (struct reg_err *err, uint32_t enable_max_abs_err, float delayed_ref, float meas);
#ifdef __cplusplus
}
#endif

#endif // LIBREG_ERR_H

// EOF

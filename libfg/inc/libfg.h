/*---------------------------------------------------------------------------------------------------------*\
  File:     libfg.h                                                                     Copyright CERN 2014

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

  Purpose:  Function generation library top level header file

  Contact:  cclibs-devs@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBFG_H
#define LIBFG_H

#include <stdint.h>
#include <math.h>
#include <stdlib.h>     // For NULL

// Constants

#define FG_CLIP_LIMIT_FACTOR    0.001
#define FG_PI                   3.141592653589793238462643383279
#define FG_ERR_DATA_LEN         4

// FG Library error numbers

enum fg_error
{
    FG_OK,
    FG_BAD_ARRAY_LEN,
    FG_BAD_PARAMETER,
    FG_INVALID_TIME,
    FG_OUT_OF_ACCELERATION_LIMITS,
    FG_OUT_OF_LIMITS,
    FG_OUT_OF_RATE_LIMITS,
    FG_OUT_OF_VOLTAGE_LIMITS
};

// Polarity of limits

enum fg_limits_polarity
{
    FG_LIMITS_POL_NORMAL,                  // Normal limits with no manipulation
    FG_LIMITS_POL_NEGATIVE,                // Limits should be inverted
    FG_LIMITS_POL_AUTO                     // Limits should be tested based upon the polarity of the reference
};

// Reference limits structure

struct fg_limits
{
    float       pos;                       // Positive reference limit
    float       min;                       // Minimum absolute reference limit
    float       neg;                       // Negative reference limit
    float       rate;                      // Rate of change limit
    float       acceleration;              // Acceleration limit

    // User callback for checking reference

    enum fg_error (*user_check_limits)(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                                       uint32_t negative_flag, float ref, float rate, float acceleration);
};

// Reference meta data structure
// NB: The fg_meta_error sub-structure is useful in case the reference function was rejected.

struct fg_meta
{
    struct fg_meta_error
    {
        uint32_t    index;                 // Error index from Init function
        float       data[FG_ERR_DATA_LEN]; // Error debug data
    } error;

    float       duration;                  // Function duration

    struct
    {
        float   start;                     // Reference value at start of function
        float   end;                       // Reference value at the end of the function
        float   min;                       // Minimum value of the function
        float   max;                       // Maximum value of the function
    } range;
};

// External functions

#ifdef __cplusplus
extern "C" {
#endif

void            fgResetMeta (struct fg_meta *meta);

enum fg_error   fgCheckRef  (struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                             uint32_t negative_flag, float ref, float rate, float acceleration,
                             struct fg_meta *meta);

#ifdef __cplusplus
}
#endif

#endif
// EOF


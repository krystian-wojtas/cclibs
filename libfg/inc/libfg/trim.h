/*---------------------------------------------------------------------------------------------------------*\
  File:     libfg/trim.h                                                                Copyright CERN 2011

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

  Purpose:  Header file for trim.c : Linear and cubic trim functions

  Contact:  cclibs-devs@cern.ch

  Notes:    Two different types of trim function are supported in the trim function family.

            The fgTrimGen function receives time as a pointer to constant float rather than as a float value.
            This allows the user to initialise an array of pointers to functions with the pointer to the
            fgTrimGen function.  If time is passed by value then the compiler promotes the float to double
            and prevents the correct initialisation.
\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBFG_TRIM_H
#define LIBFG_TRIM_H

#include "libfg.h"

// Types

enum fg_trim_type                       // Types of trim function
{
    FG_TRIM_CUBIC,
    FG_TRIM_LINEAR
};

struct fg_trim_config                   // Trim function configuration
{
    enum fg_trim_type   type;           // Type of trim
    float               duration;       // Time duration
    float               final;          // Final reference
};

struct fg_trim_pars                     // Trim function parameters
{
    float               delay;          // Time before start of function
    float               end_time;       // Time at end of function (delay + duration)
    float               time_offset;    // Timebase offset for cubic
    float               ref_initial;    // Initial reference
    float               ref_final;      // Final reference
    float               ref_offset;     // Reference offset
    float               a;              // Cubic coefficient r = a.t^3 + c.t
    float               c;              // Cubic coefficient
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

uint32_t        fgTrimGen (struct fg_trim_pars *pars, const double *time, float *ref);
enum fg_error   fgTrimInit(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                           struct fg_trim_config *config, float delay, float ref,
                           struct fg_trim_pars *pars, struct fg_meta *meta);

#ifdef __cplusplus
}
#endif

#endif
/*---------------------------------------------------------------------------------------------------------*\
  End of file: libfg/trim.h
\*---------------------------------------------------------------------------------------------------------*/

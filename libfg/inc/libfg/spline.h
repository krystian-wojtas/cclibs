/*---------------------------------------------------------------------------------------------------------*\
  File:     libfg/spline.h                                                              Copyright CERN 2011

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

  Purpose:  Header file for spline.c : Spline table functions

  Contact:  cclibs-devs@cern.ch

  Notes:    The SPLINE function uses the same configuation structure as TABLE (struc fg_table_config).
            The SPLINE function will use parabolic splines to smoothly connect the points of the table.

            The fgSplineGen function receives time as a pointer to constant float rather than as a float value.
            This allows the user to initialise an array of pointers to functions with the pointer to the
            fgSplineGen function.  If time is passed by value then the compiler promotes the float to double
            and prevents the correct initialisation.
\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBFG_SPLINE_H
#define LIBFG_SPLINE_H

#include "libfg/table.h"

// Types                            // Spline function configuration uses fg_table_config

struct fg_spline_pars               // Spline function parameters
{
    uint32_t    seg_idx;            // Segment index
    uint32_t    prev_seg_idx;       // Previous segment index for which coefficients were calculated
    uintptr_t   n_elements;         // Number of elements in table
    float       delay;              // Time before start of function
    float       acc_limit;          // Acceleration limit
    float       *ref;               // Table reference values
    float       *time;              // Table time values

    float       acc_start;          // Start of segment acceleration
    float       acc_end;            // End of segment acceleration
    float       grad_start;         // Start of segment gradient
    float       grad_spline;        // Spline connection gradient
    float       grad_end;           // Start of segment gradient
    float       spline_time;        // Time within segment of spline connection
    float       seg_duration;       // Segment duration
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

void            fgSplineCalc (struct fg_spline_pars *pars);
uint32_t        fgSplineGen  (struct fg_spline_pars *pars, const double *time, float *ref);
enum fg_error   fgSplineInit (struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                              struct fg_table_config *config, float delay, float min_time_step,
                              struct fg_spline_pars *pars, struct fg_meta *meta);

#ifdef __cplusplus
}
#endif

#endif
/*---------------------------------------------------------------------------------------------------------*\
  End of file: libfg/spline.h
\*---------------------------------------------------------------------------------------------------------*/

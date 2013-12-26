/*---------------------------------------------------------------------------------------------------------*\
  File:     inc/libfg/ramp.h                                                            Copyright CERN 2014

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

  Purpose:  Header file for ramp.c : Fast ramp based on Parabola - Parabola function

  Contact:  cclibs-devs@cern.ch

  Notes:    Parabolic - Parabolic function with time shift when rate limited

            The RAMP function is special amongst all the functions in libfg because it 
            uses the ref passed by pointer from the previous iteration to adjust the function
            time. This allows a smooth parabolic end to the function, even if the function
            was rate limited by the calling application.
\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBFG_RAMP_H
#define LIBFG_RAMP_H

#include "libfg.h"

// Constants

#define FG_RAMP_N_SEGS          2           // Number of segments: P-P -> 2

// Types

struct fg_ramp_config                       // RAMP function configuration
{
    float       final;                      // Final reference
    float       linear_rate;                // Linear rate limit
    float       acceleration;               // Absolute acceleration of the 1st parabolic segment (must be positive)
    float       deceleration;               // Absolute deceleration of the 2nd parabolic segment (must be positive)
};

struct fg_ramp_pars                         // RAMP function parameters
{
    uint32_t    pos_ramp_flag;              // Positive ramp flag
    uint32_t    pre_ramp_flag;              // Pre-ramp flag. Set if before point of inflexion of 1st parabola
    uint32_t    iteration_idx;              // Iteration index - used to suppress rate limiter for 2 iterations
    float       period;                     // Iteration period
    float       delay;                      // Time before start of function
    float       acceleration;               // Parabolic acceleration
    float       deceleration;               // Parabolic deceleration
    float       linear_rate;                // Linear rate limit
    float       ref[FG_RAMP_N_SEGS+1];      // End of segment references
    float       time[FG_RAMP_N_SEGS+1];     // End of segment times
    float       prev_ramp_ref;              // Function ref from previous iteration
    float       prev_returned_ref;          // Returned ref from previous iteration
    double      prev_time;                  // Time from previous iteration
    double      time_shift;                 // Time shift
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

void            fgRampCalc(struct fg_ramp_config *config, struct fg_ramp_pars *pars,
                           float delay, float init_ref, float init_rate, struct fg_meta *meta);
uint32_t        fgRampGen (struct fg_ramp_pars *pars, const double *time, float *ref);
enum fg_error   fgRampInit(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                           struct fg_ramp_config *config, float delay, float ref,
                           struct fg_ramp_pars *pars, struct fg_meta *meta);

#ifdef __cplusplus
}
#endif

#endif
// EOF


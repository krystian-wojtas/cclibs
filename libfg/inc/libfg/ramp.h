/*---------------------------------------------------------------------------------------------------------*\
  File:     libfg/pp.h                                                                Copyright CERN 2014

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

  Purpose:  Header file for pp.c : Parabola - Parabola functions

  Contact:  cclibs-devs@cern.ch

  Notes:    PP = Parabolic - Parabolic function with time shift when rate limited

            The PP function is special amongst all the functions in libfg because it can
            uses the ref passed by reference for the previous iteration to adjust the function
            time. This allows a smooth parabolic end to the function, even if the function
            was rate limited by the calling application.

            The fgPpGen function receives time as a pointer to constant double rather than as a double value.
            This allows the user to initialise an array of pointers to functions with the pointer to the
            fgPpGen function.  If time is passed by value then the compiler promotes the float to double
            and prevents the correct initialisation.
\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBFG_PP_H
#define LIBFG_PP_H

#include "libfg.h"

// Constants

#define FG_PP_N_SEGS          2             // Number of segments: P-P -> 2

// Types

struct fg_pp_config                         // PP function configuration
{
    float       final;                      // Final reference
    float       acceleration;               // Absolute acceleration of the parabolic segments (must be strictly positive)
};

struct fg_pp_pars                           // PP function parameters
{
    uint32_t    pos_ramp_flag;              // Positive ramp flag (PP must be inverted for positive ramps)
    float       delay;                      // Time before start of function
    float       acceleration;               // Parabolic acceleration
    float       deceleration;               // Parabolic deceleration (for now this equals acceleration)
    float       ref[FG_PP_N_SEGS+1];        // End of segment normalised references
    float       time[FG_PP_N_SEGS+1];       // End of segment times
    float       offset;                     // Reference offset = 2.ref[2]
    float       prev_pp_ref;                // Function ref from previous iteration
    float       prev_returned_ref;          // Returned ref from previous iteration
    double      prev_time;                  // Time from previous iteration
    double      time_shift;                 // Time shift
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

void            fgPpCalc(struct fg_pp_config *config, struct fg_pp_pars *pars,
                         float delay, float init_ref, struct fg_meta *meta);
uint32_t        fgPpGen (struct fg_pp_pars *pars, const double *time, float *ref);
enum fg_error   fgPpInit(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                         struct fg_pp_config *config, float delay, float ref,
                         struct fg_pp_pars *pars, struct fg_meta *meta);

#ifdef __cplusplus
}
#endif

#endif
// EOF

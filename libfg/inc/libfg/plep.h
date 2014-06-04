/*---------------------------------------------------------------------------------------------------------*\
  File:     libfg/plep.h                                                                Copyright CERN 2014

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

  Purpose:  Header file for plep.c : PLEP functions

  Contact:  cclibs-devs@cern.ch

  Notes:    PLEP = Parabolic - Linear - Exponential - Parabolic function

            The PLEP function is special because it can be initialised with a non-zero initial rate of
            change.  To do this it has the function fgPlepCalc() which accepts an initial ref and initial
            rate of change of ref. The final ref can also have a non-zero rate of change. If the final
            rate of change is not zero, then this adds a fifth parabolic segment.  This can be an
            extension of the fourth parabola, or it can have the opposite acceleration.
\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBFG_PLEP_H
#define LIBFG_PLEP_H

#include "libfg.h"

// Constants

#define FG_PLEP_N_SEGS          5           // Number of segments: P-L-E-P-P -> 5

// Types

struct fg_plep_config                       // PLEP function configuration
{
    float       final;                      // Final reference
    float       acceleration;               // Acceleration of the parabolic segments (absolute value is used)
    float       linear_rate;                // Maximum linear rate (absolute value is used)
    float       final_rate;                 // Final rate of change
    float       exp_tc;                     // Exponential time constant
    float       exp_final;                  // End reference of exponential segment (can be zero)
};

struct fg_plep_pars                         // PLEP function parameters
{
    float       normalisation;              // 1.0 for descending ramps, -1.0 for ascending ramps
    float       delay;                      // Time before start of function (s)
    float       acceleration;               // Parabolic acceleration/deceleration
    float       final_acc;                  // Normalised final parabolic acceleration
    float       linear_rate;                // Linear rate of change (always negative)
    float       final_rate;                 // Normalised final linear rate of change
    float       ref_exp;                    // Initial reference for exponential segment
    float       inv_exp_tc;                 // Time constant for exponential segment
    float       exp_final;                  // End reference of exponential segment
    float       ref[FG_PLEP_N_SEGS+1];      // End of segment normalised references
    float       time[FG_PLEP_N_SEGS+1];     // End of segment times
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

void            fgPlepCalc(struct fg_plep_config *config, struct fg_plep_pars *pars, float delay,
                           float init_ref, float init_rate, struct fg_meta *meta);
uint32_t        fgPlepGen (struct fg_plep_pars *pars, const double *time, float *ref);
enum fg_error   fgPlepInit(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                           struct fg_plep_config *config, float delay, float ref,
                           struct fg_plep_pars *pars, struct fg_meta *meta);

#ifdef __cplusplus
}
#endif

#endif
// EOF


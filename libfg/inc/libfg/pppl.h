/*---------------------------------------------------------------------------------------------------------*\
  File:     libfg/pppl.h                                                                Copyright CERN 2011

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

  Purpose:  Header file for pppl.c : PPPL functions

  Contact:  cclibs-devs@cern.ch

  Notes:    PPPL = Parabolic - Parabolic - Parabolic - Linear function

            The PPPL function allows a series of plateaus to be linked by smooth parabolic accelerations
            and decelerations.

            The fgPpplGen function receives time as a pointer to constant float rather than as a float value.
            This allows the user to initialise an array of pointers to functions with the pointer to the
            fgPpplGen function.  If time is passed by value then the compiler promotes the float to double
            and prevents the correct initialisation.
\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBFG_PPPL_H
#define LIBFG_PPPL_H

#include "libfg.h"

// Constants

#define FG_MAX_PPPLS            8               // Must match FGC_MAX_PPPLS constant in FGC XML
#define FG_PPPL_N_SEGS          4               // Number of segments per P-P-P-L -> 4

// Types

struct fg_pppl_config                           // PPPL function configuration
{

    float       acceleration1[FG_MAX_PPPLS];    // Acceleration of first  (parabolic) segment
    float       acceleration2[FG_MAX_PPPLS];    // Acceleration of second (parabolic) segment
    float       acceleration3[FG_MAX_PPPLS];    // Acceleration of third  (parabolic) segment
    float       rate2        [FG_MAX_PPPLS];    // Rate of change at start of second (parabolic) segment
    float       rate4        [FG_MAX_PPPLS];    // Rate of change of fourth (linear) segment
    float       ref4         [FG_MAX_PPPLS];    // Reference at start of fourth (linear) segment
    float       duration4    [FG_MAX_PPPLS];    // Duration of fourth (linear) segment

    uint32_t    numels_acceleration1;
    uint32_t    numels_acceleration2;
    uint32_t    numels_acceleration3;
    uint32_t    numels_rate2;
    uint32_t    numels_rate4;
    uint32_t    numels_ref4;
    uint32_t    numels_duration4;
};

struct fg_pppl_pars                             // PPPL function parameters
{
    uint32_t    seg_idx;                        // Current segment index
    uint32_t    num_segs;                       // Total number of segments (4*number of PPPLs)
    float       delay;                          // Time before start of function
    float       ref_initial;                    // Initial reference
    float       time[FG_PPPL_N_SEGS*FG_MAX_PPPLS]; // Times of the end of each segment
    float       a0  [FG_PPPL_N_SEGS*FG_MAX_PPPLS]; // ref = a2*t*t + a1*t + a0, where t is time in the
    float       a1  [FG_PPPL_N_SEGS*FG_MAX_PPPLS]; // segment (always negative since t=0 corresponds to
    float       a2  [FG_PPPL_N_SEGS*FG_MAX_PPPLS]; // the end of the segment)
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

uint32_t        fgPpplGen (struct fg_pppl_pars *pars, const double *time, float *ref);
enum fg_error   fgPpplInit(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                           struct fg_pppl_config *config, float delay, float ref,
                           struct fg_pppl_pars *pars, struct fg_meta *meta);
#ifdef __cplusplus
}
#endif

#endif
/*---------------------------------------------------------------------------------------------------------*\
  End of file: libfg/pppl.h
\*---------------------------------------------------------------------------------------------------------*/

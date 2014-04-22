/*---------------------------------------------------------------------------------------------------------*\
  File:     libfg/test.h                                                                Copyright CERN 2014

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

  Purpose:  Header file for test.c : Test (SINE, COSINE, STEPS, SQUARE) functions

  Contact:  cclibs-devs@cern.ch

  Notes:    Four different types of test function are supported in the test function family.

            The fgTestGen function receives time as a pointer to constant float rather than as a float value.
            This allows the user to initialise an array of pointers to functions with the pointer to the
            fgTestGen function.  If time is passed by value then the compiler promotes the float to double
            and prevents the correct initialisation.
\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBFG_TEST_H
#define LIBFG_TEST_H

#include <libfg.h>

// Types

enum fg_test_type                           // Types of test function
{
    FG_TEST_UNDEFINED,
    FG_TEST_COSINE,
    FG_TEST_SINE,
    FG_TEST_SQUARE,
    FG_TEST_STEPS
};

struct fg_test_config                       // Test function configuration
{
    enum fg_test_type   type;               // Type of test function
    float               amplitude_pp;       // Ref peak-to-peak amplitude
    float               num_cycles;         // Number of cycles/steps (this is rounded to nearest integer)
    float               period;             // Period
    uint32_t            window_flag;        // Window control (Enabled or Disabled)
};

struct fg_test_pars                         // Test function parameters
{
    enum fg_test_type   type;               // Type of test function
    uint32_t            window_flag;        // Window control (Enabled or Disabled)
    float               delay;              // Time before start of function
    float               end_time;           // Time at end of function (delay + duration)
    float               frequency;          // 1 / period
    float               half_period;        // period / 2
    float               ref_initial;        // Initial reference
    float               ref_final;          // Final reference after last cycle
    float               ref_amp;            // Reference amplitude
    float               duration;           // period * number of cycles
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

uint32_t        fgTestGen (struct fg_test_pars *pars, const double *time, float *ref);
enum fg_error   fgTestInit(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                           struct fg_test_config *config, float delay, float ref,
                           struct fg_test_pars *pars, struct fg_meta *meta);

#ifdef __cplusplus
}
#endif

#endif
// EOF

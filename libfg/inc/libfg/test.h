/*!
 * @file    test.h
 * @brief   Generate test functions (STEPS, SQUARE, SINE or COSINE)
 *
 * Four different types of test function are supported in the test function family.
 * 
 * fgTestGen() receives time as a pointer to constant float rather than as a
 * float value. This allows the user to initialise an array of pointers to
 * functions with the pointer to fgTestGen(). If time is passed by value then
 * the compiler promotes the float to double and prevents the correct initialisation.
 *
 * <h2>Contact</h2>
 *
 * cclibs-devs@cern.ch
 *
 * <h2>Copyright</h2>
 *
 * Copyright CERN 2014. This project is released under the GNU Lesser General
 * Public License version 3.
 * 
 * <h2>License</h2>
 *
 * This file is part of libfg.
 *
 * libfg is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBFG_TEST_H
#define LIBFG_TEST_H

#include <libfg.h>

// Types

/*!
 * Types of test function
 */
enum fg_test_type
{
    FG_TEST_UNDEFINED,
    FG_TEST_COSINE,
    FG_TEST_SINE,
    FG_TEST_SQUARE,
    FG_TEST_STEPS
};

/*!
 * Test function configuration
 */
struct fg_test_config
{
    enum fg_test_type   type;               //!< Type of test function
    float               amplitude_pp;       //!< Ref peak-to-peak amplitude
    float               num_cycles;         //!< Number of cycles/steps. This is rounded to the nearest integer.
    float               period;             //!< Period
    uint32_t            use_window;         //!< Window control. !0 to use window for sine & cosine.
};

/*!
 * Test function parameters
 */
struct fg_test_pars
{
    enum fg_test_type   type;               //!< Type of test function
    uint32_t            use_window;         //!< Window control. !0 to use window for sine & cosine.
    float               delay;              //!< Time before start of function
    float               end_time;           //!< Time at end of function (delay + duration)
    float               frequency;          //!< 1 / period
    float               half_period;        //!< period / 2
    float               ref_initial;        //!< Initial reference
    float               ref_final;          //!< Final reference after last cycle
    float               ref_amp;            //!< Reference amplitude
    float               duration;           //!< period * number of cycles
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

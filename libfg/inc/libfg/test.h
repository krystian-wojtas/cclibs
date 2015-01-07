/*!
 * @file    test.h
 * @brief   Generate test functions (STEPS, SQUARE, SINE or COSINE)
 *
 * Four types of test function are supported in the test function family:
 *
 * <ul>
 * <li>STEPS:  The reference is stepped up or down from its initial value to
 *             the initial value + amplitude_pp in equal-sized steps.</li>
 * <li>SQUARE: Creates a square wave of peak-peak amplitude amplitude_pp
 *             offset from the initial value.</li>
 * <li>SINE:   Creates a sine wave of amplitude amplitude_pp at
 *             each period. If use_window != 0, the curve is
 *             smoothed at the beginning of the first and end of the last period.</li>
 * <li>COSINE: creates a cosine wave with the same behaviour as SINE.</li>
 * </ul>
 * 
 * <h2>Contact</h2>
 *
 * cclibs-devs@cern.ch
 *
 * <h2>Copyright</h2>
 *
 * Copyright CERN 2015. This project is released under the GNU Lesser General
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
 * Test function parameters
 */
struct fg_test
{
    double              delay;              //!< Time before start of function.
    enum fg_test_type   type;               //!< Type of test function.
    bool                is_window_active;   //!< Window control: true to use window for sine & cosine.
    uint32_t            num_cycles;         //!< Number of cycles or steps.
    float               duration;           //!< period * number of cycles.
    float               frequency;          //!< 1 / period.
    float               half_period;        //!< period / 2.
    float               initial_ref;        //!< Initial reference.
    float               final_ref;          //!< Final reference after last cycle.
    float               amplitude;          //!< Reference amplitude.
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

/*!
 * Initialise TEST function.
 *
 * @param[in]  limits              Pointer to fgc_limits structure (or NULL if no limits checking required).
 * @param[in]  is_pol_switch_auto  True if polarity switch can be changed automatically.
 * @param[in]  is_pol_switch_neg   True if polarity switch is currently in the negative position.
 * @param[in]  delay               Delay before the start of the function.
 * @param[in]  type                Type of test function.
 * @param[in]  initial_ref         Initial reference value.
 * @param[in]  amplitude_pp        Peak-to-peak amplitude.
 * @param[in]  num_cycles          Number of cycles/steps. This is rounded to the nearest integer.
 * @param[in]  period              Period.
 * @param[in]  is_window_active    Window control: true to use window for sine & cosine.
 * @param[out] pars                Pointer to test function parameters.
 * @param[out] meta                Pointer to diagnostic information. Set to NULL if not required.
 *
 * @retval FG_OK on success
 * @retval FG_BAD_PARAMETER if invalid function type requested
 * @retval FG_INVALID_TIME if total time is too long (\f$>10^{6}s\f$)
 * @retval FG_OUT_OF_LIMITS if reference value exceeds limits
 * @retval FG_OUT_OF_RATE_LIMITS if rate of change of reference exceeds limits
 * @retval FG_OUT_OF_ACCELERATION_LIMITS if acceleration exceeds limits
 */
enum fg_error fgTestInit(struct fg_limits *limits, 
                         bool   is_pol_switch_auto,
                         bool   is_pol_switch_neg,
                         double delay, 
                         enum   fg_test_type type,
                         float  initial_ref,
                         float  amplitude_pp,
                         float  num_cycles,
                         float  period,
                         bool   is_window_active,
                         struct fg_test *pars, 
                         struct fg_meta *meta);



/*!
 * Generate the reference for the Test functions.
 *
 * @param[in]  pars             Pointer to test function parameters.
 * @param[in]  time             Pointer to current time within the function.
 * @param[out] ref              Pointer to new reference value.
 *
 * @retval FG_GEN_BEFORE_FUNC   if time is before the start of the function.
 * @retval FG_GEN_DURING_FUNC   if time is during the function.
 * @retval FG_GEN_AFTER_FUNC    if time is after the end of the function.
 */
enum fg_gen_status fgTestGen(struct fg_test *pars, const double *time, float *ref);

#ifdef __cplusplus
}
#endif

#endif

// EOF

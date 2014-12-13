/*!
 * @file    test.h
 * @brief   Generate test functions (STEPS, SQUARE, SINE or COSINE)
 *
 * Four types of test function are supported in the test function family:
 *
 * <ul>
 * <li>STEPS:  the reference is stepped up or down from its initial value to
 *             fg_test_config::amplitude_pp in equal-sized steps.</li>
 * <li>SQUARE: creates a square wave of amplitude fg_test_config::amplitude_pp
 *             at each period.</li>
 * <li>SINE:   creates a sine wave of amplitude fg_test_config::amplitude_pp at
 *             each period. If fg_test_config::use_window != 0, the curve is
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
    bool                is_window_active;   //!< Window control: true to use window for sine & cosine.
};

/*!
 * Test function parameters
 */
struct fg_test_pars
{
    enum fg_test_type   type;               //!< Type of test function
    bool                is_window_active;   //!< Window control: true to use window for sine & cosine.
    double              delay;              //!< Time before start of function
    uint32_t            num_cycles;         //!< Number of cycles or steps.
    float               duration;           //!< period * number of cycles
    float               frequency;          //!< 1 / period
    float               half_period;        //!< period / 2
    float               ref_initial;        //!< Initial reference
    float               ref_final;          //!< Final reference after last cycle
    float               ref_amp;            //!< Reference amplitude
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

/*!
 * Check Test function configuration and initialise parameters.
 *
 * @param[in]  limits           Limits to check each segment against
 * @param[in]  limits_polarity  Polarity limits to check each segment against
 * @param[in]  config           Test configuration parameters
 * @param[in]  delay            RUN_DELAY, delay before the start of the function
 * @param[in]  init_ref         Initial reference value
 * @param[out] pars             Test function parameters
 * @param[out] meta             Diagnostic information. Set to NULL if not required.
 *
 * @retval FG_OK on success
 * @retval FG_BAD_PARAMETER if invalid function type requested
 * @retval FG_INVALID_TIME if total time is too long (\f$>10^{6}s\f$)
 * @retval FG_OUT_OF_LIMITS if reference value exceeds limits
 * @retval FG_OUT_OF_RATE_LIMITS if rate of change of reference exceeds limits
 * @retval FG_OUT_OF_ACCELERATION_LIMITS if acceleration exceeds limits
 */
enum fg_error fgTestInit(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                         struct fg_test_config *config, double delay, float init_ref,
                         struct fg_test_pars *pars, struct fg_meta *meta);

/*!
 * Generate the reference for the Test functions.
 *
 * @param[in]  pars             Test function parameters
 * @param[in]  time             Current time within the function. Note that time
 *                              is passed by const reference rather than by value.
 *                              This allows the user to initialise an array of
 *                              pointers to functions with the pointer to
 *                              fgTestGen(). If time is passed by value then the
 *                              compiler promotes the float to double and prevents
 *                              correct initialisation.
 * @param[out] ref              Derived reference value
 *
 * @retval false    if time is beyond the end of the function.
 * @retval true     if time is before the end of the function.
 */
bool fgTestGen(struct fg_test_pars *pars, const double *time, float *ref);

#ifdef __cplusplus
}
#endif

#endif

// EOF

/*!
 * @file    table.h
 * @brief   Generate linearly interpolated table functions.
 *
 * This function is provided with a list of reference values and times that the
 * reference values take effect, i.e. it calculates the reference by table lookup.
 *
 * Using linear interpolation results in an error between the actual linear
 * reference and the ideal parabolic reference. It can be shown that for parabola
 * \f$y = \frac{at^{2}}{2}\f$, the maximum interpolation error for segment
 * \f$0 \leq t \leq T\f$ is \f$\epsilon_{max} = \frac{aT^{2}}{8}\f$.
 *
 * \image html  Interpolation_error.png
 * \image latex Interpolation_error.png "Linear interpolation error" width=0.5\textwidth
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

#ifndef LIBFG_TABLE_H
#define LIBFG_TABLE_H

#include "libfg.h"

// Types

/*!
 * Table function configuration
 */
struct fg_table_config
{
    float       *ref;               //!< Table reference values
    float       *time;              //!< Table time values
    uintptr_t   ref_n_elements;     //!< Number of elements in reference array
    uintptr_t   time_n_elements;    //!< Number of elements in time array
};

/*!
 * Table function parameters
 */
struct fg_table_pars
{
    uint32_t    seg_idx;            //!< Segment index
    uint32_t    prev_seg_idx;       //!< Previous segment index for which gradient was calculated
    uintptr_t   n_elements;         //!< Number of elements in table
    float       delay;              //!< Time before start of function
    float       *ref;               //!< Table reference values
    float       *time;              //!< Table time values
    float       seg_grad;           //!< Gradient of reference for segment fg_table_pars::prev_seg_idx
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

/*!
 * Check Table function configuration and initialise parameters.
 *
 * @param[in]  limits           Limits to check each segment against
 * @param[in]  limits_polarity  Polarity limits to check each segment against
 * @param[in]  config           Table configuration parameters
 * @param[in]  delay            RUN_DELAY, delay before the start of the function
 * @param[in]  min_time_step    Minimum time between interpolation points
 * @param[out] pars             Table function parameters
 * @param[out] meta             Diagnostic information. Set to NULL if not required.
 *
 * @retval FG_OK on success
 * @retval FG_BAD_ARRAY_LEN if there are less than two points, or elements of config are different lengths
 * @retval FG_INVALID_TIME if first time value is not zero, or time values are not at least min_time_step seconds apart
 * @retval FG_OUT_OF_LIMITS if reference value exceeds limits
 * @retval FG_OUT_OF_RATE_LIMITS if rate of change of reference exceeds limits
 * @retval FG_OUT_OF_ACCELERATION_LIMITS if acceleration exceeds limits
 */
enum fg_error fgTableInit(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                          struct fg_table_config *config, float delay, float min_time_step,
                          struct fg_table_pars *pars, struct fg_meta *meta);

/*!
 * Generate the reference for the Table function.
 *
 * @param[in]  pars             Table function parameters
 * @param[in]  time             Current time within the function. Note that time
 *                              is passed by const reference rather than by value.
 *                              This allows the user to initialise an array of
 *                              pointers to functions with the pointer to
 *                              fgTableGen(). If time is passed by value then the
 *                              compiler promotes the float to double and prevents
 *                              correct initialisation.
 * @param[out] ref              Derived reference value
 *
 * @retval 0 if time is beyond the end of the function
 * @retval 1 if time is within a segment (or before the start of the first segment)
 */
uint32_t fgTableGen(struct fg_table_pars *pars, const double *time, float *ref);

#ifdef __cplusplus
}
#endif

#endif

// EOF

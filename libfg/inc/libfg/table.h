/*!
 * @file    table.h
 * @brief   Generate linearly interpolated table functions.
 *
 * This function is provided with a list of reference values and times that the
 * reference values take effect, i.e. it calculates the reference by table lookup.
 *
 * Note that if the calling application Using linear interpolation results in an error between the actual linear
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

#ifndef LIBFG_TABLE_H
#define LIBFG_TABLE_H

#include "libfg.h"

/*!
 * Table function parameters
 */
struct fg_table
{
    double      delay;              //!< Time before start of function.
    uint32_t    seg_idx;            //!< Segment index.
    uint32_t    prev_seg_idx;       //!< Previous segment index for which gradient was calculated.
    uint32_t    num_points;         //!< Number of points in table.
    float       *ref;               //!< Table reference values.
    float       *time;              //!< Table time values.
    float       seg_grad;           //!< Gradient of reference for segment fg_table::prev_seg_idx.
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

/*!
 * Initialise TABLE function.
 *
 * @param[in]  limits             Pointer to fgc_limits structure (or NULL if no limits checking required).
 * @param[in]  is_pol_switch_auto True if polarity switch can be changed automatically.
 * @param[in]  is_pol_switch_neg  True if polarity switch is currently in the negative position.
 * @param[in]  delay              Delay before the start of the function.
 * @param[in]  min_time_step      Minimum time between points in the table.
 * @param[in] *ref                Array of reference values.
 * @param[in]  ref_num_els        Number of elements in reference array.
 * @param[in] *time               Array of time values.
 * @param[in]  time_num_els       Number of elements in time array.
 * @param[out] pars               Pointer to table function parameters.
 * @param[out] meta               Pointer to diagnostic information. Set to NULL if not required.
 *
 * @retval FG_OK on success
 * @retval FG_BAD_ARRAY_LEN if there are less than two points, or ref and time arrays are different lengths
 * @retval FG_INVALID_TIME if first time value is not zero, or time values are not at least min_time_step seconds apart
 * @retval FG_OUT_OF_LIMITS if reference value exceeds limits
 * @retval FG_OUT_OF_RATE_LIMITS if rate of change of reference exceeds limits
 * @retval FG_OUT_OF_ACCELERATION_LIMITS if acceleration exceeds limits
 */
enum fg_error fgTableInit(struct   fg_limits *limits, 
                          bool     is_pol_switch_auto,
                          bool     is_pol_switch_neg,
                          double   delay, 
                          float    min_time_step,
                          float   *ref,
                          uint32_t ref_num_els,
                          float   *time,
                          uint32_t time_num_els,
                          struct   fg_table *pars, 
                          struct   fg_meta *meta);



/*!
 * Generate the reference for the Table function.
 *
 * @param[in]  pars             Pointer to table function parameters.
 * @param[in]  time             Pointer to time within the function.
 * @param[out] ref              Pointer to reference value.
 *
 * @retval FG_GEN_BEFORE_FUNC   if time is before the start of the function.
 * @retval FG_GEN_DURING_FUNC   if time is during the function.
 * @retval FG_GEN_AFTER_FUNC    if time is after the end of the function.
 */
enum fg_gen_status fgTableGen(struct fg_table *pars, const double *time, float *ref);

#ifdef __cplusplus
}
#endif

#endif

// EOF

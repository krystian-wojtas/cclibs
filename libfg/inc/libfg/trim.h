/*!
 * @file    trim.h
 * @brief   Generate linear and cubic trim functions
 *
 * Two types of trim function are supported, linear and cubic. It is possible to 
 * define the duration of the trim, or to go as fast as possible while respecting
 * the limits. For more details, see <a href="../pdf/TRIMS.pdf">TRIMS.pdf</a>.
 *
 * <h2>CTRIM or Cubic Trim (fg_trim_type::FG_TRIM_CUBIC)</h2>
 *
 * The Cubic Trim reference function is \f$r = at^{3} + ct\f$
 *
 * \image html  Cubic_Trim.png
 * \image latex Cubic_Trim.png "Cubic Trim" width=0.5\textwidth
 *
 * <h2>LTRIM or Linear Trim (fg_trim_type::FG_TRIM_LINEAR)</h2>
 *
 * The Linear Trim reference function is \f$r = ct\f$
 *
 * \image html  Linear_Trim.png
 * \image latex Linear_Trim.png "Linear Trim" width=0.5\textwidth
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

#ifndef LIBFG_TRIM_H
#define LIBFG_TRIM_H

#include "libfg.h"

// Types

/*!
 * Types of trim function
 */
enum fg_trim_type
{
    FG_TRIM_UNDEFINED,
    FG_TRIM_CUBIC,
    FG_TRIM_LINEAR
};

/*!
 * Trim function parameters
 */
struct fg_trim
{
    double              delay;          //!< Time before start of function.
    float               duration;       //!< Function duration.
    float               time_offset;    //!< Timebase offset for cubic.
    float               ref_offset;     //!< Reference offset.
    float               initial_ref;    //!< Initial reference.
    float               final_ref;      //!< Final reference.
    float               a;              //!< Coefficient for cubic term. \f$r = a \cdot t^3 + c \cdot t\f$
    float               c;              //!< Coefficient for constant term. \f$r = a \cdot t^3 + c \cdot t\f$
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

/*!
 * Initialise TRIM function.
 *
 * @param[in]  limits              Pointer to fgc_limits structure (or NULL if no limits checking required).
 * @param[in]  is_pol_switch_auto  True if polarity switch can be changed automatically.
 * @param[in]  is_pol_switch_neg   True if polarity switch is currently in the negative position.
 * @param[in]  delay               Delay before the start of the function.
 * @param[in]  initial_ref         Initial reference value.
 * @param[in]  final_ref           Final reference value.
 * @param[in]  type                Type of trim function (LINEAR or CUBIC).
 * @param[in]  duration            Function duration (Zero to go as fast as limits allow).
 * @param[out] pars                Pointer to trim function parameters.
 * @param[out] meta                Pointer to diagnostic information. Set to NULL if not required.
 *
 * @retval FG_OK on success
 * @retval FG_BAD_PARAMETER on parameter errors
 * @retval FG_OUT_OF_LIMITS if reference value exceeds limits
 * @retval FG_OUT_OF_RATE_LIMITS if rate of change of reference exceeds limits
 * @retval FG_OUT_OF_ACCELERATION_LIMITS if acceleration exceeds limits
 */
enum fg_error fgTrimInit(struct fg_limits *limits, 
                         bool   is_pol_switch_auto,
                         bool   is_pol_switch_neg,
                         double delay, 
                         enum   fg_trim_type type,
                         float  initial_ref,
                         float  final_ref,
                         float  duration,
                         struct fg_trim *pars, 
                         struct fg_meta *meta);



/*!
 * Generate the reference for the Trim function.
 *
 * @param[in]  pars            Pointer to trim function parameters.
 * @param[in]  time            Pointer to time within the function.
 * @param[out] ref             Pointer to reference value.
 *
 * @retval FG_GEN_BEFORE_FUNC   if time is before the start of the function.
 * @retval FG_GEN_DURING_FUNC   if time is during the function.
 * @retval FG_GEN_AFTER_FUNC    if time is after the end of the function.
 */
enum fg_gen_status fgTrimGen (struct fg_trim *pars, const double *time, float *ref);

#ifdef __cplusplus
}
#endif

#endif

// EOF

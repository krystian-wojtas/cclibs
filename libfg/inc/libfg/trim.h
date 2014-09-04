/*!
 * @file    trim.h
 * @brief   Generate linear and cubic trim functions
 *
 * Two types of trim function are supported, linear and cubic. For more details, see
 * <a href="../pdf/TRIMS.pdf">TRIMS.pdf</a>.
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
 * Trim function configuration
 */
struct fg_trim_config
{
    enum fg_trim_type   type;           //!< Type of trim
    float               duration;       //!< Time duration
    float               final;          //!< Final reference
};

/*!
 * Trim function parameters
 */
struct fg_trim_pars
{
    float               delay;          //!< Time before start of function
    float               end_time;       //!< Time at end of function (delay + duration)
    float               time_offset;    //!< Timebase offset for cubic
    float               ref_initial;    //!< Initial reference
    float               ref_final;      //!< Final reference
    float               ref_offset;     //!< Reference offset
    float               a;              //!< Coefficient for cubic term. \f$r = a \cdot t^3 + c \cdot t\f$
    float               c;              //!< Coefficient for constant term. \f$r = a \cdot t^3 + c \cdot t\f$
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

/*!
 * Check Trim configuration and initialise parameters for each segment.
 *
 * @param[in]  limits           Limits to check each segment against
 * @param[in]  limits_polarity  Polarity limits to check each segment against
 * @param[in]  config           Trim configuration parameters
 * @param[in]  delay            RUN_DELAY, delay before the start of the function
 * @param[in]  ref              Initial reference value
 * @param[out] pars             Trim function parameters for each segment
 * @param[out] meta             Diagnostic information. Set to NULL if not required.
 *
 * @retval FG_OK on success
 * @retval FG_BAD_PARAMETER on parameter errors
 * @retval FG_OUT_OF_LIMITS if reference value exceeds limits
 * @retval FG_OUT_OF_RATE_LIMITS if rate of change of reference exceeds limits
 * @retval FG_OUT_OF_ACCELERATION_LIMITS if acceleration exceeds limits
 */
enum fg_error fgTrimInit(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                           struct fg_trim_config *config, float delay, float ref,
                           struct fg_trim_pars *pars, struct fg_meta *meta);

/*!
 * Generate the reference for the Trim function.
 *
 * @param[in]  pars            Trim function parameters for each segment
 * @param[in]  time            Current time within the function. Note that time
 *                             is passed by const reference rather than by value.
 *                             This allows the user to initialise an array of
 *                             pointers to functions with the pointer to
 *                             fgTrimGen(). If time is passed by value then the
 *                             compiler promotes the float to double and prevents
 *                             correct initialisation.
 * @param[out] ref             Derived reference value
 *
 * @retval 0 if time is beyond the end of the function
 * @retval 1 if time is within the function (or before the start of the function)
 */
uint32_t fgTrimGen (struct fg_trim_pars *pars, const double *time, float *ref);

#ifdef __cplusplus
}
#endif

#endif

// EOF

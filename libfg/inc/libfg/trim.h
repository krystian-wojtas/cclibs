/*!
 * @file    trim.h
 * @brief   Generate linear and cubic trim functions
 *
 * Two different types of trim function are supported in the trim
 * function family.
 * 
 * fgTrimGen() receives time as a pointer to constant float rather than as a
 * float value. This allows the user to initialise an array of pointers to
 * functions with the pointer to fgTrimGen(). If time is passed by value then
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

uint32_t        fgTrimGen (struct fg_trim_pars *pars, const double *time, float *ref);
enum fg_error   fgTrimInit(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                           struct fg_trim_config *config, float delay, float ref,
                           struct fg_trim_pars *pars, struct fg_meta *meta);

#ifdef __cplusplus
}
#endif

#endif
// EOF

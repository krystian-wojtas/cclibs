/*!
 * @file  err.h
 * @brief Converter Control Regulation library regulation error functions
 *
 * Functions for all types of regulation (current, field, voltage). These
 * functions maintain a history of the reference so that the measurement can be
 * compared against the reference, taking into account the tracking delay.
 *
 * <h2>Copyright</h2>
 *
 * Copyright CERN 2014. This project is released under the GNU Lesser General
 * Public License version 3.
 * 
 * <h2>License</h2>
 *
 * This file is part of libreg.
 *
 * libreg is free software: you can redistribute it and/or modify it under the
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

#ifndef LIBREG_ERR_H
#define LIBREG_ERR_H

#include <stdint.h>

/*!
 * Regulation error limit structure
 */
struct reg_err_limit
{
    float                       threshold;                      //!< Limit threshold
    float                       filter;                         //!< Threshold exceeded flag filter (counter)
    uint32_t                    flag;                           //!< Limit exceeded flag
};

/*!
 * Regulation error structure
 */
struct reg_err
{
    uint32_t                    inhibit_max_abs_err_counter;    //!< Down counter to inhibit max_abs_err calculation
    float                       delayed_ref;                    //!< Delayed reference
    float                       err;                            //!< Regulation error
    float                       max_abs_err;                    //!< Max absolute error
    struct reg_err_limit        warning;                        //!< Warning limit structure
    struct reg_err_limit        fault;                          //!< Fault limit structure
};
/*!
 * Regulation error macros
 */
#define regErrInhibitMaxAbsErrRT(err,iterations) (err)->inhibit_max_abs_err_counter=iterations

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Initialise the warning and fault limits of the reg_err structure.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]   err                  Pointer to regulation error structure.
 * @param[in]    warning_threshold    New warning threshold. If set to zero, reset
 *                                    reg_err_limit::flag and reg_err_limit::filter in reg_err::warning.
 * @param[in]    fault_threshold      New fault threshold. If set to zero, reset
 *                                    reg_err_limit::flag and reg_err_limit::filter in reg_err::fault.
 */
void regErrInitLimits(struct reg_err *err, float warning_threshold, float fault_threshold);

/*!
 * Reset the reg_err structure variables to zero.
 * <strong>Note:</strong> reg_err::max_abs_err is not reset.
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[out]   err                            Pointer to regulation error structure.
 * @param[in]    inhibit_max_abs_err_counter    Number of calls to regErrCheckLimitsRT() during which the max_abs_err
 *                                              calculation should be inhibited.
 */
void regErrResetLimitsVarsRT(struct reg_err *err, uint32_t inhibit_max_abs_err_counter);

/*!
 * Calculate the regulation error and check the error limits (if supplied).
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in,out] err                Pointer to regulation error structure
 * @param[in]     delayed_ref        Value to store in reg_err::delayed_ref, so it can be logged if required
 * @param[in]     meas               reg_err::err is set to <em>delayed_ref - meas</em>.
 */
void regErrCheckLimitsRT(struct reg_err *err, float delayed_ref, float meas);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_ERR_H

// EOF

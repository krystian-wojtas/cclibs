/*!
 * @file  ramp.h
 * @brief Generate fast ramp based on Parabola-Parabola function.
 * 
 * RAMP is a special function within libfg in two way:
 *
 * 1. It can start with a non-zero rate of change (by calling fgRampCalc())
 * 2. It can responds to rate of change limits in the calling application.
 *
 * The first feature allows the RAMP function to take over from an a running function.
 * 
 * The second feature allow a RAMP to go as fast as possible, with limits applied
 * by the calling application, while still arriving smoothly at the final value.
 * Note that the function duration returned in fg_meta::meta will be wrong if the
 * reference is limited at any time during the generation of the function. 
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

#ifndef LIBFG_RAMP_H
#define LIBFG_RAMP_H

#include "libfg.h"

// Constants

#define FG_RAMP_NUM_SEGS        2           //!< Number of segments: P-P = 2

/*!
 * RAMP function parameters
 */
struct fg_ramp
{
    double      delay;                      //!< Time before start of function.
    bool        is_ramp_positive;           //!< Positive ramp flag.
    bool        is_pre_ramp;                //!< Pre-ramp flag. True if before point of inflexion of 1st parabola.
    float       initial_ref;                //!< Reference before the start of the function.
    float       acceleration;               //!< Parabolic acceleration.
    float       deceleration;               //!< Parabolic deceleration.
    float       linear_rate;                //!< User linear rate.
    float       linear_rate_limit;          //!< Actual linear rate limit.
    float       ref [FG_RAMP_NUM_SEGS+1];   //!< End of segment references. See also #FG_RAMP_NUM_SEGS.
    float       time[FG_RAMP_NUM_SEGS+1];   //!< End of segment times. See also #FG_RAMP_NUM_SEGS.
    float       prev_ramp_ref;              //!< Function ref from previous iteration
    float       prev_returned_ref;          //!< Returned ref from previous iteration
    double      prev_time;                  //!< Time from previous iteration
    double      time_shift;                 //!< Time shift
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

/*!
 * Initialise RAMP function and check limits.
 *
 * This function uses fgRampCalc() to prepare the Ramp parameters.
 *
 * @param[in]  limits             Pointer to fgc_limits structure (or NULL if no limits checking required).
 * @param[in]  is_pol_switch_auto True if polarity switch can be changed automatically.
 * @param[in]  is_pol_switch_neg  True if polarity switch is currently in the negative position.
 * @param[in]  delay              Delay before the start of the function.
 * @param[in]  initial_ref        Initial reference value.
 * @param[in]  final_ref          Final reference value.
 * @param[in]  acceleration       Acceleration of the 1st parabolic segment. Absolute value is used.
 * @param[in]  linear_rate        Maximum linear rate. Absolute value is used.
 * @param[in]  deceleration       Deceleration of the 2nd parabolic segment. Absolute value is used.
 * @param[out] pars               Pointer to Ramp function parameters.
 * @param[out] meta               Pointer to diagnostic information. Set to NULL if not required.
 *
 * @retval FG_OK on success
 * @retval FG_BAD_PARAMETER if acceleration == 0 or deceleration == 0
 * @retval FG_OUT_OF_LIMITS if reference value exceeds limits
 * @retval FG_OUT_OF_RATE_LIMITS if rate of change of reference exceeds limits
 * @retval FG_OUT_OF_ACCELERATION_LIMITS if acceleration exceeds limits
 */
enum fg_error fgRampInit(struct fg_limits *limits, 
                         bool   is_pol_switch_auto,
                         bool   is_pol_switch_neg,
                         double delay, 
                         float  initial_ref,
                         float  final_ref,
                         float  acceleration,
                         float  linear_rate,
                         float  deceleration,
                         struct fg_ramp *pars, 
                         struct fg_meta *meta);



/*!
 * Initialise RAMP function without checking limits.
 *
 * This function is used by fgRampInit(). It does no limit checking. It is a separate function so 
 * a real-time application can initialise a ramp, for example, to take over from another running function.
 * To allow this, the function accepts an initial rate of change.
 *
 * @param[in]  is_pol_switch_auto True if polarity switch can be changed automatically.
 * @param[in]  is_pol_switch_neg  True if polarity switch is currently in the negative position.
 * @param[in]  delay              Delay before the start of the function.
 * @param[in]  init_rate          Initial rate of change.
 * @param[in]  initial_ref        Initial reference value.
 * @param[in]  final_ref          Final reference value.
 * @param[in]  acceleration       Acceleration of the 1st parabolic segment. Absolute value is used.
 * @param[in]  linear_rate        Maximum linear rate. Absolute value is used.
 * @param[in]  deceleration       Deceleration of the 2nd parabolic segment. Absolute value is used.
 * @param[out] pars               Pointer to ramp function parameters. 
 *                                The coordinates of the transition points between
 *                                the segments of the ramp function are:
 *                                <ul>
 *                                <li>pars->time[0], pars->ref[0]: Max/min of first (accelerating) parabola</li>
 *                                <li>pars->time[1], pars->ref[1]: Connection between accelerating and decelerating parabolas</li>
 *                                <li>pars->time[2], pars->ref[2]: End of the second (decelerating) parabola, also end of the ramp function</li>
 *                                </ul>
 * @param[out] meta               Pointer to diagnostic information structure. Set to NULL if not required.
 */
void fgRampCalc(bool   is_pol_switch_auto,
                bool   is_pol_switch_neg,
                double delay, 
                float  init_rate,
                float  initial_ref,
                float  final_ref,
                float  acceleration,
                float  linear_rate,
                float  deceleration,
                struct fg_ramp *pars, struct fg_meta *meta);



/*!
 * Generate the reference for the Ramp function.
 *
 * Derive the reference for the previously-initialised Ramp function at the given time.
 *
 * <strong>NOTE</strong>: Unlike the other libfg functions, TIME MUST NOT GO BACKWARDS.
 *
 * @param[in]     pars             Pointer to ramp function parameters.
 * @param[in]     time             Pointer to time within the function. 
 * @param[in,out] ref              Pointer to the reference value. If the application needed to clip
 *                                 the reference, fgRampGen() will take this into account.
 *
 * @retval FG_GEN_BEFORE_FUNC   if time is before the start of the function.
 * @retval FG_GEN_DURING_FUNC   if time is during the function.
 * @retval FG_GEN_AFTER_FUNC    if time is after the end of the function.
 */
enum fg_gen_status fgRampGen(struct fg_ramp *pars, const double *time, float *ref);

#ifdef __cplusplus
}
#endif

#endif

// EOF

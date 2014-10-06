/*!
 * @file  ramp.h
 * @brief Generate fast ramp based on Parabola-Parabola function.
 * 
 * RAMP is a special function which responds by adjusting a time-shift if the
 * reference is rate-limited by the calling application. The reference is passed
 * by pointer from iteration to iteration. This effectively slows the reference
 * time, allowing a smooth parabolic end to the function, even if the function
 * was rate-limited by the calling application. The function will continue
 * smoothly when the reference is no longer limited.
 *
 * Note that the function duration returned in fg_meta::meta will be wrong if the
 * reference is limited at any time during the generation of the function. Also,
 * time must never go backwards.
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

#ifndef LIBFG_RAMP_H
#define LIBFG_RAMP_H

#include "libfg.h"

// Constants

#define FG_RAMP_N_SEGS          2           //!< Number of segments: P-P = 2

// Types

/*!
 * RAMP function configuration
 */
struct fg_ramp_config
{
    float       final;                      //!< Final reference
    float       acceleration;               //!< Acceleration of the 1st parabolic segment. Absolute value is used.
    float       linear_rate;                //!< Maximum linear rate. Absolute value is used.
    float       deceleration;               //!< Deceleration of the 2nd parabolic segment. Absolute value is used.
};

/*!
 * RAMP function parameters
 */
struct fg_ramp_pars
{
    uint32_t    pos_ramp_flag;              //!< Positive ramp flag
    uint32_t    pre_ramp_flag;              //!< Pre-ramp flag. Set if before point of inflexion of 1st parabola
    float       init_ref;                   //!< Reference before the start of the function
    float       delay;                      //!< Time before start of function
    float       acceleration;               //!< Parabolic acceleration
    float       deceleration;               //!< Parabolic deceleration
    float       linear_rate;                //!< User linear rate
    float       linear_rate_limit;          //!< Actual linear rate limit
    float       ref [FG_RAMP_N_SEGS+1];     //!< End of segment references. See also #FG_RAMP_N_SEGS.
    float       time[FG_RAMP_N_SEGS+1];     //!< End of segment times. See also #FG_RAMP_N_SEGS.
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
 * Check Ramp function configuration and initialise parameters.
 *
 * @param[in]  limits           Limits to check each segment against
 * @param[in]  limits_polarity  Polarity limits to check each segment against
 * @param[in]  config           Ramp configuration parameters
 * @param[in]  delay            RUN_DELAY, delay before the start of the function
 * @param[in]  ref              Initial reference value
 * @param[out] pars             Ramp function parameters
 * @param[out] meta             Diagnostic information. Set to NULL if not required.
 *
 * @retval FG_OK on success
 * @retval FG_BAD_PARAMETER if fg_ramp_pars::acceleration == 0 or fg_ramp_pars::deceleration == 0
 * @retval FG_OUT_OF_LIMITS if reference value exceeds limits
 * @retval FG_OUT_OF_RATE_LIMITS if rate of change of reference exceeds limits
 * @retval FG_OUT_OF_ACCELERATION_LIMITS if acceleration exceeds limits
 */
enum fg_error fgRampInit(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                         struct fg_ramp_config *config, float delay, float ref,
                         struct fg_ramp_pars *pars, struct fg_meta *meta);

/*!
 * Generate the reference for the Ramp function.
 *
 * Derive the reference for the previously-initialised Ramp function at the given time.
 *
 * <strong>NOTE</strong>: Unlike the other libfg functions, TIME MUST NOT GO BACKWARDS.
 *
 * @param[in]  pars             Ramp function parameters, containing the
 *                              coordinates of the transition points between the
 *                              segments of the ramp function:
 *                              <ul>
 *                              <li>pars->time[0], pars->ref[0]: Max/min of first (accelerating) parabola</li>
 *                              <li>pars->time[1], pars->ref[1]: Connection between accelerating and decelerating parabolas</li>
 *                              <li>pars->time[2], pars->ref[2]: End of the second (decelerating) parabola, also end of the ramp function</li>
 *                              </ul>
 * @param[in]  time             Current time within the function. Note that time
 *                              is passed by const reference rather than by value.
 *                              This allows the user to initialise an array of
 *                              pointers to functions with the pointer to
 *                              fgRampGen(). If time is passed by value then the
 *                              compiler promotes the float to double and prevents
 *                              correct initialisation.
 * @param[out] ref              Pointer to the reference value from the previous
 *                              iteration. This may have been clipped if any
 *                              limits were reached.
 *
 * @retval 0 if time is beyond the end of the function.
 * @retval 1 if time is within the function.
 */
uint32_t fgRampGen(struct fg_ramp_pars *pars, const double *time, float *ref);

/*!
 * Calculate Ramp function parameters.
 *
 * @param[in]     config        Ramp configuration parameters
 * @param[in]     delay         RUN_DELAY, delay before the start of the function
 * @param[in]     init_ref      Initial reference value
 * @param[in]     init_rate     Initial linear rate of change
 * @param[out]    pars          Ramp function parameters. The parameters from the
 *                              previous iteration are used to calculate the
 *                              parameters for the current iteration.
 *                              <em>pars</em> is updated with the new parameters.
 *                              The coordinates of the transition points between
 *                              the segments of the ramp function are:
 *                              <ul>
 *                              <li>pars->time[0], pars->ref[0]: Max/min of first (accelerating) parabola</li>
 *                              <li>pars->time[1], pars->ref[1]: Connection between accelerating and decelerating parabolas</li>
 *                              <li>pars->time[2], pars->ref[2]: End of the second (decelerating) parabola, also end of the ramp function</li>
 *                              </ul>
 * @param[out]    meta          Diagnostic information. Set to NULL if not required.
 */
void fgRampCalc(struct fg_ramp_config *config, float delay, float init_ref, float init_rate,
                struct fg_ramp_pars *pars, struct fg_meta *meta);

#ifdef __cplusplus
}
#endif

#endif

// EOF

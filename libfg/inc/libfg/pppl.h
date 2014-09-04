/*!
 * @file    pppl.h
 * @brief   Generate Parabola-Parabola-Parabola-Linear (PPPL) functions
 *
 * The PPPL function allows a series of plateaus to be linked by smooth
 * parabolic accelerations and decelerations.
 *
 * <h2>PPPL Reference Definition</h2>
 *
 * <ul>
 * <li>Seven parameters define each PPPL section from four segments</li>
 * <li>Up to eight PPPL sections can be chained together</li>
 * <li>If ACCELERATION2 is zero, it becomes a PLPL</li>
 * <li>Ramps can be up or down</li>
 * </ul>
 *
 * \image html  PPPL_parameters.png
 * \image latex PPPL_parameters.png "PPPL Reference Definition" width=\textwidth
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBFG_PPPL_H
#define LIBFG_PPPL_H

#include "libfg.h"

// Constants

/*!
 * Up to 8 PPPL sections can be chained together. Must match FGC_MAX_PPPLS constant in FGC XML.
 */
#define FG_MAX_PPPLS            8
/*!
 * Each PPPL section contains 4 segments (P-P-P-L)
 */
#define FG_PPPL_N_SEGS          4

// Types

/*!
 * PPPL function configuration
 */
struct fg_pppl_config
{
    float       acceleration1[FG_MAX_PPPLS];    //!< Acceleration of first  (parabolic) segment. See also #FG_MAX_PPPLS.
    float       acceleration2[FG_MAX_PPPLS];    //!< Acceleration of second (parabolic) segment. See also #FG_MAX_PPPLS.
    float       acceleration3[FG_MAX_PPPLS];    //!< Acceleration of third  (parabolic) segment. See also #FG_MAX_PPPLS.
    float       rate2        [FG_MAX_PPPLS];    //!< Rate of change at start of second (parabolic) segment. See also #FG_MAX_PPPLS.
    float       rate4        [FG_MAX_PPPLS];    //!< Rate of change of fourth (linear) segment. See also #FG_MAX_PPPLS.
    float       ref4         [FG_MAX_PPPLS];    //!< Reference at start of fourth (linear) segment. See also #FG_MAX_PPPLS.
    float       duration4    [FG_MAX_PPPLS];    //!< Duration of fourth (linear) segment. See also #FG_MAX_PPPLS.

    uint32_t    numels_acceleration1;           //!< Number of elements in fg_pppl_config::acceleration1
    uint32_t    numels_acceleration2;           //!< Number of elements in fg_pppl_config::acceleration2
    uint32_t    numels_acceleration3;           //!< Number of elements in fg_pppl_config::acceleration3
    uint32_t    numels_rate2;                   //!< Number of elements in fg_pppl_config::rate2
    uint32_t    numels_rate4;                   //!< Number of elements in fg_pppl_config::rate4
    uint32_t    numels_ref4;                    //!< Number of elements in fg_pppl_config::ref4
    uint32_t    numels_duration4;               //!< Number of elements in fg_pppl_config::duration4
};

/*!
 * PPPL function parameters.
 * \f$ref = a_{2} \cdot t^{2} + a_{1} \cdot t + a_{0}\f$,
 * where \f$t\f$ is time in the segment (always negative, since \f$t=0\f$
 * corresponds to the end of the segment).
 */
struct fg_pppl_pars
{
    uint32_t    seg_idx;                           //!< Current segment index
    uint32_t    num_segs;                          //!< Total number of segments (4*number of PPPLs)
    float       delay;                             //!< Time before start of function
    float       ref_initial;                       //!< Initial reference
    float       time[FG_PPPL_N_SEGS*FG_MAX_PPPLS]; //!< Times of the end of each segment. See also #FG_PPPL_N_SEGS and #FG_MAX_PPPLS.
    float       a0  [FG_PPPL_N_SEGS*FG_MAX_PPPLS]; //!< Coefficient for constant term. \f$ref = a_{2} \cdot t^{2} + a_{1} \cdot t + a_{0}\f$
    float       a1  [FG_PPPL_N_SEGS*FG_MAX_PPPLS]; //!< Coefficient for linear term. \f$ref = a_{2} \cdot t^{2} + a_{1} \cdot t + a_{0}\f$
    float       a2  [FG_PPPL_N_SEGS*FG_MAX_PPPLS]; //!< Coefficient for quadratic term. \f$ref = a_{2} \cdot t^{2} + a_{1} \cdot t + a_{0}\f$
};

#ifdef __cplusplus
extern "C" {
#endif

// External functions

/*!
 * Check PPPL configuration and initialise parameters for each segment.
 *
 * @param[in]  limits           Limits to check each segment against
 * @param[in]  limits_polarity  Polarity limits to check each segment against
 * @param[in]  config           PPPL configuration parameters
 * @param[in]  delay            RUN_DELAY, delay before the start of the function
 * @param[in]  ref              Initial reference value
 * @param[out] pars             PPPL function parameters for each segment
 * @param[out] meta             Diagnostic information. Set to NULL if not required.
 *
 * @retval FG_OK on success
 * @retval FG_BAD_ARRAY_LEN if elements of config are different lengths
 * @retval FG_BAD_PARAMETER on any other parameter error
 * @retval FG_INVALID_TIME if any segments have negative duration
 * @retval FG_OUT_OF_LIMITS if reference value exceeds limits
 * @retval FG_OUT_OF_RATE_LIMITS if rate of change of reference exceeds limits
 * @retval FG_OUT_OF_ACCELERATION_LIMITS if acceleration exceeds limits
 */
enum fg_error fgPpplInit(struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                         struct fg_pppl_config *config, float delay, float ref,
                         struct fg_pppl_pars *pars, struct fg_meta *meta);

/*!
 * Generate the reference for the PPPL function.
 *
 * @param[in]  pars            PPPL function parameters for each segment
 * @param[in]  time            Current time within the function. Note that time
 *                             is passed by const reference rather than by value.
 *                             This allows the user to initialise an array of
 *                             pointers to functions with the pointer to
 *                             fgPpplGen(). If time is passed by value then the
 *                             compiler promotes the float to double and prevents
 *                             correct initialisation.
 * @param[out] ref             Derived reference value
 *
 * @retval 0 if time is beyond the end of the function
 * @retval 1 if time is within a segment (or before the start of the first segment)
 */
uint32_t fgPpplGen(struct fg_pppl_pars *pars, const double *time, float *ref);

#ifdef __cplusplus
}
#endif

#endif

// EOF

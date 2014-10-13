/*!
 * @file  lim.h
 * @brief Converter Control Regulation library limit functions for field/current/voltage reference and field/current measurement
 *
 * The limits support includes three types of limits relevant to power converter controls:
 * 
 * <ol>
 * <li>Field/Current measurement limits (trip)</li>
 * <li>Field/Current reference limits (clip)</li>
 * <li>Voltage reference limits (clip)</li>
 * </ol>
 *
 * Voltage reference limits for some 4-quadrant converters need to protect against
 * excessive power losses in the output stage when ramping down the current. This
 * can be done by defining an exclusion zone for positive voltages in quadrants 4
 * and 1. The software will rotate the zone by @htmlonly 180&deg; @endhtmlonly to
 * define the exclusion zone for negative voltages in quadrants 3 and 2. For example:
 *
 * \image html  4QuadrantLimits.png
 * \image latex 4QuadrantLimits.png "Exclusion Zones for 4-Quadrant Converters" width=\textwidth
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

#ifndef LIBREG_LIM_H
#define LIBREG_LIM_H

#include <stdint.h>

// Constants

#define REG_LIM_CLIP            0.001                           //!< Clip limit shift factor
#define REG_LIM_TRIP            0.01                            //!< Trip limit shift factor
#define REG_LIM_HYSTERESIS      0.1                             //!< Low/Zero limit hysteresis factor
#define REG_LIM_FP32_MARGIN     2.0E-07                         //!< Margin on the relative precision of 32-bit floats

// Limits structures

/*!
 * Measurement limits
 */
struct reg_lim_meas
{
    enum reg_enabled_disabled   invert_limits;                  //!< Flag to invert limits before use.
                                                                //!< (<em>e.g.</em>, polarity switch is negative)
    float                       pos_trip;                       //!< Positive measurement trip limit
    float                       neg_trip;                       //!< Negative measurement trip limit
    float                       low;                            //!< Low measurement threshold
    float                       zero;                           //!< Zero measurement threshold
    float                       low_hysteresis;                 //!< Low measurement threshold with hysteresis
    float                       zero_hysteresis;                //!< Zero measurement threshold with hysteresis

    struct
    {
        enum reg_enabled_disabled   trip;                       //!< Set if measurement exceeds reg_lim_meas::pos_trip or reg_lim_meas::neg_trip limits
        enum reg_enabled_disabled   low;                        //!< Set if absolute measurement is below reg_lim_meas::low
        enum reg_enabled_disabled   zero;                       //!< Set if absolute measurement is below reg_lim_meas::zero
    } flags;                                                    //!< Measurement limit flags
};

/*!
 * Reference limits
 */
struct reg_lim_ref
{
    enum reg_enabled_disabled   invert_limits;                  //!< Flag to invert limits before use.
                                                                //!< (<em>e.g.</em>, polarity switch is negative)
    float                       pos;                            //!< User's positive limit
    float                       min;                            //!< User's min limit

    float                       max_clip;                       //!< Maximum reference clip limit from reg_lim_ref::max_clip_user or Q41 limit
    float                       min_clip;                       //!< Minimum reference clip limit from reg_lim_ref::min_clip_user or Q41 limit
    float                       rate_clip;                      //!< Absolute reference rate clip limit

    float                       max_clip_user;                  //!< Maximum reference clip limit from user
    float                       min_clip_user;                  //!< Minimum reference clip limit from user

    float                       closeloop;                      //!< Closeloop threshold (0 for bipolar reference)

    float                       i_quadrants41_max;              //!< Quadrants 41 exclusion zone. At least a 1A spread is needed to
                                                                //!< activate the Q41 limiter. Disable by setting to -1.0E10.
    float                       v0;                             //!< Voltage limit for zero measured current
    float                       dvdi;                           //!< Voltage limit slope with measured current

    struct
    {
        enum reg_enabled_disabled   unipolar;                   //!< Unipolar flag
        enum reg_enabled_disabled   clip;                       //!< Set if reference has been clipped to range
                                                                //!< [reg_lim_ref::min_clip,reg_lim_ref::max_clip]
        enum reg_enabled_disabled   rate;                       //!< Set if reference rate has been clipped to range
                                                                //!< [- reg_lim_ref::rate_clip,reg_lim_ref::rate_clip]
    } flags;                                                    //!< Reference limit flags
};

/*!
 * RMS limits
 */
struct reg_lim_rms
{
    float                       rms2_fault;                     //!< Squared RMS fault threshold
    float                       rms2_warning;                   //!< Squared RMS warning threshold
    float                       rms2_warning_hysteresis;        //!< Squared RMS warning threshold with hysteresis
    float                       meas2_filter;                   //!< Filtered square of the measurement
    float                       meas2_filter_factor;            //!< First-order filter factor for square of measurement

    struct
    {
        enum reg_enabled_disabled   fault;                      //!< Set if filtered square of the measurement exceeds reg_lim_rms::rms2_fault
        enum reg_enabled_disabled   warning;                    //!< Set if filtered square of the measurement exceeds reg_lim_rms::rms2_warning
    } flags;                                                    //!< RMS limits flags
};

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Initialise measurement limits structure.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    lim_meas         Measurement limits object to initialise
 * @param[in]     pos_lim          Positive measurement trip limit. Will be scaled by (1+#REG_LIM_TRIP)
 * @param[in]     neg_lim          Negative measurement trip limit. Will be scaled by (1+#REG_LIM_TRIP)
 * @param[in]     low_lim          Low measurement threshold
 * @param[in]     zero_lim         Zero measurement threshold
 */
void regLimMeasInit(struct reg_lim_meas *lim_meas, float pos_lim, float neg_lim, float low_lim, float zero_lim);

/*!
 * Initialise measurement limits structure RMS squared trip and warning thresholds.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    lim_meas         Measurement limits object to initialise
 * @param[in]     rms_warning      RMS warning threshold
 * @param[in]     rms_fault        RMS fault threshold
 * @param[in]     rms_tc           Used to determine whether reg_lim_meas::rms2_trip, reg_lim_meas::rms2_warning
 *                                 and reg_lim_meas::rms2_warning_hysteresis should be set, and to calculate the
 *                                 value for reg_lim_meas::meas2_filter_factor.
 * @param[in]     iter_period      Iteration period, used to calculate the value for reg_lim_meas::meas2_filter_factor.
 */
void regLimRmsInit(struct reg_lim_rms *lim_rms, float rms_warning, float rms_fault, float rms_tc, double iter_period);

/*!
 * Initialise field/current reference limits. Field/current limits use the same
 * structure as voltage reference limits but have different behaviour.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    lim_ref          Reference limits object to initialise
 * @param[in]     pos_lim          Maximum reference clip limit. Will be scaled by (1+#REG_LIM_CLIP).
 * @param[in]     min_lim          Standby reference limit.
 * @param[in]     neg_lim          Minimum reference clip limit. Will be scaled by (1+#REG_LIM_CLIP).
 * @param[in]     rate_lim         Absolute reference rate clip limit. Will be scaled by (1+#REG_LIM_CLIP).
 * @param[in]     closeloop        Closeloop threshold.
 */
void regLimRefInit(struct reg_lim_ref *lim_ref, float pos_lim, float min_lim, float neg_lim, float rate_lim, float closeloop);

/*!
 * Initialise voltage reference limits. Voltage reference limits use the same
 * structure as field/current limits but have different behaviour.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    lim_v_ref        Voltage reference limits object to initialise
 * @param[in]     pos_lim          Maximum reference clip limit. Will be scaled by (1+#REG_LIM_CLIP)
 * @param[in]     neg_lim          Minimum reference clip limit. Will be scaled by (1+#REG_LIM_CLIP)
 * @param[in]     rate_lim         Absolute reference rate clip limit. Will be scaled by (1+#REG_LIM_CLIP)
 * @param[in]     i_quadrants41    Define exclusion zone in quadrants 4 and 1 (I dimension)
 * @param[in]     v_quadrants41    Define exclusion zone in quadrants 4 and 1 (V dimension)
 */
void regLimVrefInit(struct reg_lim_ref *lim_v_ref, float pos_lim, float neg_lim, float rate_lim,
                    float i_quadrants41[2], float v_quadrants41[2]);

/*!
 * Check the measurement against the trip levels and the absolute measurement against
 * the low and zero limits with hysteresis to avoid toggling. The flags in reg_lim_meas::flags
 * are set according to the result of the checks. This function calls regLimVrefCalcRT().
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in,out] lim_meas         Measurement limits object to check against
 * @param[in]     meas             Measurement value
 */
void regLimMeasRT(struct reg_lim_meas *lim_meas, float meas);

/*!
 * Check filtered squared measurement against limits.
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in,out] lim_rms          Pointer to RMS current limits structure
 * @param[in]     i_meas           Measured current
 */
void regLimMeasRmsRT(struct reg_lim_rms *lim_rms, float meas);

/*!
 * Use the measured current to work out the voltage limits based on the operating
 * zone for the voltage source. The user defines the exclusion zone for positive
 * voltages (quadrants 4-1) in regLimVrefInit(). This function rotates this
 * zone to calculate the exclusion zone for negative voltages (quadrants 3-2).
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in,out] lim_v_ref        Voltage reference limits object
 * @param[in]     i_meas           Measured current
 */
void regLimVrefCalcRT(struct reg_lim_ref *lim_v_ref, float i_meas);

/*!
 * Apply clip and rate limits to the field, current or voltage reference. The appropriate
 * flags in reg_lim_ref::flags are set if clip or rate limits are applied.
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in,out] lim_ref          Reference limits object to update
 * @param[in]     period           Iteration period, used to compute rate limit
 * @param[in]     ref              Reference value to check
 * @param[in]     prev_ref         Previous reference, used to calculate change of reference value
 */
float regLimRefRT(struct reg_lim_ref *lim_ref, float period, float ref, float prev_ref);

#ifdef __cplusplus
}
#endif

// inline function definitions

static inline void regLimMeasInvert(struct reg_lim_meas *lim_meas, enum reg_enabled_disabled invert_limits)
{
    lim_meas->invert_limits = invert_limits;
}

static inline void regLimRefInvert(struct reg_lim_ref *lim_ref, enum reg_enabled_disabled invert_limits)
{
    lim_ref->invert_limits = invert_limits;
}

#endif // LIBREG_LIM_H

// EOF

/*!
 * @file  rst.h
 * @brief Converter Control Regulation library RST regulation algorithm functions
 *
 * <h2>The RST Algorithm for Closed Loop Regulation</h2>
 *
 * The RST algorithm satisfies:
 *
 * \f[
 * \sum_{0}^{N} \{ Act_{i} \cdot S_{i} \} = \sum_{0}^{N} \{ Ref_{i} \cdot T_{i} \} - \sum_{0}^{N} \{ Meas_{i} \cdot R_{i} \}
 * \f]
 *
 * where \f$Act\f$ is the actuation, \f$Ref\f$ is the reference and \f$Meas\f$ is the measurement;
 * \f$i=0\f$ corresponds to the current sample, \f$i=1\f$ corresponds to the previous sample, <em>etc.</em>
 *
 * Libreg uses Landau notation for the RST algorithm; the other common notation (Longchamp) swaps
 * the R and S polynomials.
 *
 * For a magnet circuit, the reference and measurement can be of the circuit current or the magnetic field (see #reg_mode).
 * The actuation defines the circuit voltage which the voltage source must try to follow. By keeping the history of the
 * previous N samples of the reference, measurement and actuation, and knowing the new reference and measurement, the new
 * actuation is calculated as:
 *
 * \f[
 * Act_{0} = \frac{\sum_{0}^{N} \{ Ref_{i} \cdot T_{i} \} - \sum_{0}^{N} \{ Meas_{i} \cdot R_{i} \} - \sum_{1}^{N} \{ Act_{i} \cdot S_{i} \}}{S_{0}}
 * \f]
 *
 * When the actuation is limited or is being driven in open-loop, then the reference for the current actuation and measurement
 * is back-calculated as:
 *
 * \f[
 * Ref_{0} = \frac{\sum_{0}^{N} \{ Act_{i} \cdot S_{i} \} + \sum_{0}^{N} \{ Meas_{i} \cdot R_{i} \} - \sum_{1}^{N} \{ Ref_{i} \cdot T_{i} \}}{T_{0}}
 * \f]
 *
 * This is used to keep the reference history consistent with the measurement and actuation histories. Back-calculation of the
 * reference is a simple way to implement the anti-windup behaviour found in traditional regulation algorithms.
 *
 * In summary, if you know any two of { Actuation, Reference, Measurement }, you can calculate the third as illustrated below:
 *
 * \image html  RST.png
 * \image latex RST.png "If you know two, you can calculate the third" width=\textwidth
 *
 * The benefit of the RST equation is that any linear regulator up to order N can be implemented by choosing the appropriate
 * RST polynomial coefficients. Simple PI, PID or PII controllers can be implemented as well as more %complex higher order
 * systems, without changing the software. For %complex higher-order systems, calculating the coefficients is a challenge.
 * This is beyond the scope of Libreg, but coefficients calculated by an expert (<em>e.g.</em> using Matlab) can be supplied,
 * see regRstInit(). Libreg can calculate the coefficients itself if the following conditions are met:
 *
 * <ul>
 * <li>The required bandwidth of the current or field regulation is much less than the bandwidth of the voltage source.</li>
 * <li>The bandwidth of the reference is less than the bandwidth of the regulation.</li>
 * </ul>
 *
 * The algorithm implements a deadbeat PII regulator by ignoring the voltage response and modeling the load as documented in
 * load.h. Being a deadbeat controller, this will have a track delay of exactly 1 regulation period, for constant rates of
 * change of the reference. Note that the regulation bandwidth defines the bandwidth for the rejection of perturbations, not
 * the bandwidth for tracking.
 *
 * <h2>The Open Loop Regulation Algorithm</h2>
 *
 * For the load model described in load.h, the following differential equation must hold true:
 * \f[
 * (R_p + R_m)V(t) + L_m\frac{dV}{dt}(t) = \left ( R_c(R_p + R_m) + R_p\cdot R_m \right ) I_{cir}(t) + (R_p + R_c)L_m\frac{dI_{cir}}{dt}(t)
 * \f]
 * We simplify this by combining the constant terms:
 * \f[
 * R_1 = R_p + R_m
 * \f]
 * \f[
 * R_2 = (R_c\cdot R_1 + R_p\cdot R_m)
 * \f]
 * \f[
 * R_3 = R_p + R_c
 * \f]
 * so that:
 * \f[
 * R_1\cdot V(t) + L_m\frac{dV}{dt}(t) = R_2\cdot I_{cir}(t) + R_3\cdot L_m\frac{dI_{cir}}{dt}(t)
 * \f]
 *
 *
 * This differential equation is converted into a difference equation by taking the Z transform, using a discretization
 * technique (Forward Euler, Backwards Euler or Tustin) to approximate the Laplace transform. The difference equation
 * can be manipulated algebraically and can be reduced to three coefficients, which are calculated in regRstInit() and
 * stored in reg_openloop. 
 *
 * The difference equation then becomes:
 *
 * \f$V(t) = act[1]_{forward}\cdot V(t-1) + ref[0]_{forward}\cdot I(t) + ref[1]_{forward}\cdot I(t-1)\f$
 *
 * in the forward direction (to calculate the actuation), and:
 *
 * \f$I(t) = act[0]_{reverse}\cdot V(t) + ref[0]_{reverse}\cdot I(t) + ref[1]_{reverse}\cdot I(t-1)\f$
 *
 * in the reverse direction (to back-calculate the reference).
 *
 * <h2>Floating-point Precision</h2>
 * 
 * rst.c uses 32-bit floating point for most of the floating point variables.
 * However, there are parts of the RST computation where higher precision is
 * crucial. 40-bit is sufficient &mdash; this is the level available on the
 * TI TMS320C32 DSP. On newer processors, 64-bit double precision is needed.
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

#ifndef LIBREG_RST_H
#define LIBREG_RST_H

// Constants

#define REG_NUM_RST_COEFFS         10                           //!< RST order + 1 (must be \f$\leq\f$ #REG_RST_HISTORY_MASK)
#define REG_RST_HISTORY_MASK       31                           //!< History buffer index mask (must be \f$2^{N}-1\f$)
#define REG_MM_WARNING_THRESHOLD   0.4                          //!< #REG_WARNING level for Modulus Margin

#include <stdint.h>
#include <stdbool.h>
#include <libreg.h>
#include <libreg/meas.h>
#include <libreg/load.h>

// Regulation RST algorithm structures

/*!
 * Regulation status
 */
enum reg_status
{
    REG_OK,
    REG_WARNING,
    REG_FAULT
};

/*!
 * Converter regulation mode
 */
enum reg_mode
{
    REG_VOLTAGE,                                                //!< Open loop (voltage reference)
    REG_CURRENT,                                                //!< Closed loop on current
    REG_FIELD,                                                  //!< Closed loop on field
    REG_NONE                                                    //!< No regulation mode set
};

/*!
 * RST polynomial arrays and track delay
 */
struct reg_rst
{
    float                       r[REG_NUM_RST_COEFFS];          //!< R polynomial coefficients (measurement). See also #REG_NUM_RST_COEFFS.
    float                       s[REG_NUM_RST_COEFFS];          //!< S polynomial coefficients (actuation). See also #REG_NUM_RST_COEFFS.
    float                       t[REG_NUM_RST_COEFFS];          //!< T polynomial coefficients (reference). See also #REG_NUM_RST_COEFFS.
};

/*!
 * Openloop coefficients. The difference equation for open loop regulation is:
 *
 * \f$V(t) = act[1]_{forward}\cdot V(t-1) + ref[0]_{forward}\cdot I(t) + ref[1]_{forward}\cdot I(t-1)\f$
 *
 * in the forward direction (to calculate the actuation), and:
 *
 * \f$I(t) = act[0]_{reverse}\cdot V(t) + ref[0]_{reverse}\cdot I(t) + ref[1]_{reverse}\cdot I(t-1)\f$
 *
 * in the reverse direction (to back-calculate the reference).
 *
 * These coefficients are calculated in regRstInit()
 */
struct reg_openloop
{
    float                       ref[2];                         //!< Difference equation coefficients for I(t) and I(t-1) terms.
    float                       act[2];                         //!< Difference equation coefficients for V(t) term (used only in the reverse
};                                                              //!< direction) and V(t-1) terms (used only in the forward direction).

/*!
 * RST algorithm parameters
 */
struct reg_rst_pars
{
    enum reg_mode               reg_mode;                       //!< Regulation mode (#REG_CURRENT | #REG_FIELD)
    float                       reg_period;                     //!< Regulation period
    float                       inv_reg_period_iters;           //!< \f$\frac{1}{reg\_period\_iters}\f$
    float                       min_auxpole_hz;                 //!< Minimum of RST auxpole*_hz parameters. Used to limit the scan frequency range.

    struct reg_openloop         openloop_forward;               //!< Coefficients for openloop difference equation in forward direction.
    struct reg_openloop         openloop_reverse;               //!< Coefficients for openloop difference equation in reverse direction.
    struct reg_rst              rst;                            //!< RST polynomials
    uint32_t                    rst_order;                      //!< Highest order of RST polynomials
    float                       inv_s0;                         //!< \f$\frac{1}{S[0]}\f$
    float                       t0_correction;                  //!< Correction to t[0] for rounding errors
    float                       inv_corrected_t0;               //!< \f$\frac{1}{T[0]+ t0\_correction}\f$

    enum reg_status             status;                         //!< Regulation parameters status
    enum reg_jurys_result       jurys_result;                   //!< Jury's test result 
    uint32_t                    alg_index;                      //!< Algorithm index (1-5). Based on pure delay
    uint32_t                    dead_beat;                      //!< 0 = not dead-beat, 1-3 = dead-beat (1-3)
    float                       ref_advance;                    //!< Reference advance time
    float                       pure_delay_periods;             //!< Pure delay in regulation periods
    float                       track_delay_periods;            //!< Track delay in regulation periods
    float                       ref_delay_periods;              //!< Reference delay in regulation periods used for regulation error calculation
    enum reg_meas_select        reg_err_meas_select;            //!< Measurement to use for regulation error calculation (FILTERED or UNFILTERED)

    float                       modulus_margin;                 //!< Modulus margin. Equal to the minimum value of the sensitivity function (abs_S_p_y)
    float                       modulus_margin_freq;            //!< Frequency for modulus margin.
    float                       a   [REG_NUM_RST_COEFFS];       //!< Plant numerator A. See also #REG_NUM_RST_COEFFS.
    float                       b   [REG_NUM_RST_COEFFS];       //!< Plant denominator B. See also #REG_NUM_RST_COEFFS.
    float                       as  [REG_NUM_RST_COEFFS];       //!< \f$A \cdot S\f$. See also #REG_NUM_RST_COEFFS.
    float                       asbr[REG_NUM_RST_COEFFS];       //!< \f$A \cdot S + B \cdot R\f$. See also #REG_NUM_RST_COEFFS.
};

/*!
 * RST algorithm variables
 */
struct reg_rst_vars
{
    uint32_t                    history_index;                  //!< Index to latest entry in the history
    float                       prev_ref_rate;                  //!< Reference rate from previous iteration

    float                       openloop_ref[REG_RST_HISTORY_MASK+1]; //!< Openloop calculated reference history. Only the two most
                                                                      //!< recent values are used. See also #REG_RST_HISTORY_MASK.
    float                       ref         [REG_RST_HISTORY_MASK+1]; //!< RST calculated reference history. See also #REG_RST_HISTORY_MASK.
    float                       meas        [REG_RST_HISTORY_MASK+1]; //!< RST measurement history. See also #REG_RST_HISTORY_MASK.
    float                       act         [REG_RST_HISTORY_MASK+1]; //!< RST actuation history. See also #REG_RST_HISTORY_MASK.
};

// RST macro "functions"

#define regRstIncHistoryIndexRT(rst_vars_p) (rst_vars_p)->history_index = ((rst_vars_p)->history_index + 1) & REG_RST_HISTORY_MASK
#define regRstPrevRefRT(rst_vars_p)         (rst_vars_p)->ref[(rst_vars_p)->history_index]
#define regRstDeltaRefRT(rst_vars_p)        (regRstPrevRefRT(rst_vars_p) - (rst_vars_p)->ref[((rst_vars_p)->history_index - 1) & REG_RST_HISTORY_MASK])
#define regRstPrevActRT(rst_vars_p)         (rst_vars_p)->act[(rst_vars_p)->history_index]
#define regRstAverageDeltaActRT(rst_vars_p) ((regRstPrevActRT(rst_vars_p) - (rst_vars_p)->act[((rst_vars_p)->history_index + 1) & REG_RST_HISTORY_MASK])/REG_RST_HISTORY_MASK)

// RST regulation functions

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Prepare coefficients for the RST regulation algorithm, to allow a pure loop delay to be accommodated.
 * This function also prepares coefficients for open loop regulation. See reg_openloop for details.
 *
 * <h3>Notes</h3>
 * 
 * The algorithms can calculate the RST coefficients from the auxpole*, pure delay and load parameters.
 * This only works well if the voltage source bandwidth and FIR notch are much faster (\f$>10\times\f$)
 * than the closed loop bandwidth. Three controllers are selectable: I, PI, PII. For the PII, the regulator
 * may or may not be dead-beat according to the pure delay.
 * 
 * If the voltage source bandwidth is less than a factor 10 above the closed loop bandwidth, then the
 * algorithms will not produce good results and the RST coefficients need to be calculated manually,
 * <em>e.g.</em> using Matlab.
 *
 * <h3>Reference</h3>
 *
 * This function is based on the paper <a href="https://edms.cern.ch/document/686163/1">CERN EDMS 686163</a>
 * by Hugues Thiesen with extensions from Martin Veenstra and Michele Martino.
 *
 * This is a background function: do not call from the real-time thread or interrupt.
 *
 * @param[out] pars                   RST parameters object to update with the new coefficients.
 * @param[in]  reg_period_iters       Regulation period. Specified as an integer number of iteration periods,
 *                                    as regulation can only run on iteration boundaries.
 * @param[in]  reg_period             Regulation period in seconds.
 * @param[in]  load                   Load parameters struct. Used to calculate RST coefficients.
 * @param[in]  auxpole1_hz            Frequency of (real) auxillary pole 1. Used to calculate RST coefficients.
 *                                    If auxpole1_hz \f$\leq 0\f$, the I, PI and PII controllers are not used
 *                                    to calculate the RST coefficients; instead, manually-calculated
 *                                    coefficients must be supplied (see the <em>manual</em> parameter below).
 * @param[in]  auxpoles2_hz           Frequency of (conjugate) auxillary poles 2 and 3. Used to calculate RST
 *                                    coefficients.
 * @param[in]  auxpoles2_z            Damping of (conjugate) auxillary poles 2 and 3. Used to calculate RST
 *                                    coefficients.
 * @param[in]  auxpole4_hz            Frequency of (real) auxillary pole 4. Used to calculate RST coefficients.
 * @param[in]  auxpole5_hz            Frequency of (real) auxillary pole 5. Used to calculate RST coefficients.
 * @param[in]  pure_delay_periods     Pure delay in the regulation loop, specified as number of periods. This
 *                                    models the voltage reference delay + measurement delay + voltage source
 *                                    response) as a simple delay. For stability, this cannot be more than 40%
 *                                    of the regulation period. Used to calculate RST coefficients.
 * @param[in]  track_delay_periods    Anticipated delay between the setting of the field/current reference and
 *                                    the moment when the measurement should equal the reference. Specified as
 *                                    a number of iterations. The track delay depends upon the type of
 *                                    regulator being used; set to 1 in the case of the dead-beat PII algorithm.
 *                                    Used to calculate the error in the response of the regulation loop.
 * @param[in]  reg_mode               Regulation mode (voltage, current or field).
 * @param[in]  manual                 Externally calculated RST parameters. These are used if auxpoles1_hz is zero.
 *
 * @retval     REG_OK on success
 * @retval     REG_WARNING if reg_rst_pars::modulus_margin < #REG_MM_WARNING_THRESHOLD
 * @retval     REG_FAULT if s[0] is too small (\f$<1\times 10^{-10}\f$) or is unstable (has poles outside the unit circle)
 */
enum reg_status regRstInit(struct reg_rst_pars *pars, uint32_t reg_period_iters, float  reg_period,
                           struct reg_load_pars *load, float auxpole1_hz, float auxpoles2_hz,
                           float auxpoles2_z, float auxpole4_hz, float auxpole5_hz,
                           float pure_delay_periods, float track_delay_periods,
                           enum reg_mode reg_mode, struct reg_rst *manual);



/*!
 * Complete the initialisation of the RST history in vars.
 *
 * The actuation (act) and measurement (meas) histories must be kept up to date. This function will initalise
 * the reference (ref) history based on the measurement history and the supplied rate of change. It will
 * modify meas[0] to balance the RST history so as to reduce the perturbation following the change of regulation
 * mode.
 *
 * This is a background function: do not call from the real-time thread or interrupt.
 *
 * @param[out]    vars          Pointer to history of actuation, measurement and reference values.
 * @param[in]     ref           Initial reference/measurement value
 * @param[in]     openloop_ref  Initial openloop reference value
 * @param[in]     act           Estimated measurement rate
 */
void regRstInitHistory(struct reg_rst_vars *vars, float ref, float openloop_ref, float act);



/*!
 * Complete the initialisation of the RST history in vars.
 *
 * The actuation (act) and measurement (meas) histories must be kept up to date. This function will initalise
 * the reference (ref) history based on the measurement history and the supplied rate of change. It will
 * modify meas[0] to balance the RST history so as to reduce the perturbation following the change of regulation
 * mode.
 *
 * This is a Real-Time function.
 *
 * @param[in]     pars    Pointer to RST parameters structure
 * @param[in,out] vars    Pointer to history of actuation, measurement and reference values.
 * @param[in]     rate    Estimated measurement rate
 */
void regRstInitRefRT(struct reg_rst_pars *pars, struct reg_rst_vars *vars, float rate);



/*!
 * Use the supplied RST or open loop coefficients to calculate the actuation based on the supplied reference value
 * and the measurement. If <em>is_openloop</em> is true, then the open loop regulation algorithm is used. Otherwise,
 * we are in closed loop mode and the RST algorithm is used.
 *
 * This is a Real-Time function.
 *
 * @param[in]     pars           RST coefficients and parameters
 * @param[in,out] vars           History of actuation, measurement and reference values. Updated with new values by this function.
 * @param[in]     ref            Latest reference value
 * @param[in]     is_openloop    Set to true if the function should return the actuation calculated by the open loop algorithm.
 *                               This is required for 1- and 2-quadrant converters while the measurement is less than the
 *                               minimum current for closed loop regulation.
 *
 * @returns       New actuation value
 */
float regRstCalcActRT(struct reg_rst_pars *pars, struct reg_rst_vars *vars, float ref, bool is_openloop);



/*!
 * Use the supplied RST and open loop coefficients to back-calculate the reference based on the supplied actuation
 * value and the measurement in *vars. This function is always called after regRstCalcActRT(). It back-calculates the RST and
 * open loop references and saves them in the respective histories. If <em>is_limited</em> is true, both RST and open
 * loop references are calculated, otherwise we calculate the one we are not, based on the value of <em>is_openloop</em>.
 * The function returns the reference for the active mode (open or closed loop).
 *
 * This is a Real-Time function.
 *
 * @param[in]     pars           RST coefficients and parameters
 * @param[in,out] vars           History of actuation, measurement and reference values. Updated with new values by this function.
 * @param[in]     act            Latest actuation value
 * @param[in]     is_limited     Set to true if <em>act</em> has been limited. In this case, both closed-loop and open-loop reference
 *                               values will be calculated for the history. If set to false, we only calculate one or the other
 *                               (determined by <em>is_openloop</em>).
 * @param[in]     is_openloop    Set to true if the function should return the reference back-calculated by the open loop algorithm.
 *                               This is required for 1- and 2-quadrant converters while the measurement is less than I_CLOSELOOP.
 *
 * @returns       Back-calculated reference value
 */
void regRstCalcRefRT(struct reg_rst_pars *pars, struct reg_rst_vars *vars, float act, bool is_limited, bool is_openloop);



/*!
 * Measure the tracking delay if the reference is changing.
 * This function must be called after calling regRstCalcRefRT().
 *
 * This is a Real-Time function.
 *
 * @param[in]     vars            History of regulation actuation, measurement and reference values.
 *
 * @returns       Measured track delay in regulation period (clipped between 0.5 and 3.5)
 */
float regRstTrackDelayRT(struct reg_rst_vars *vars);



/*!
 * Calculate the delayed reference for the next iteration.
 *
 * It should be called after regRstIncHistoryIndexRT() and regErrCheckLimitsRT() have been called to prepare the delayed
 * reference for the following iteration. It can be called every acquisition iteration between regulation
 * iterations, or just on the regulation iterations, as required.
 *
 * This is a Real-Time function.
 *
 * @param[in]     pars               RST parameters. The amount of delay is specified by reg_rst_pars::ref_delay_periods and
 *                                   reg_rst_pars::inv_reg_period_iters.
 * @param[in]     vars               History of actuation, measurement and reference values.
 * @param[in]     iteration_index    Multiplier for reg_rst_pars::inv_reg_period_iters, to account for the acquisition
 *                                   iteration time between regulation iterations
 *
 * @returns       Delayed reference value.
 */
float regRstDelayedRefRT(struct reg_rst_pars *pars, struct reg_rst_vars *vars, uint32_t iteration_index);



/*!
 * Calculate the average RST actuation (V_REF) over the past #REG_AVE_V_REF_LEN iterations.
 *
 * This is a Real-Time function.
 *
 * @param[in]     vars    History of actuation values.
 *
 * @returns       Average actuation value
 */
float regRstAverageVrefRT(struct reg_rst_vars *vars);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_RST_H

// EOF

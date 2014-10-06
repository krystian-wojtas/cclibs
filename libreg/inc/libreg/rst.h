/*!
 * @file  rst.h
 * @brief Converter Control Regulation library RST regulation algorithm functions
 *
 * <h2>The RST Algorithm</h2>
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

#include <stdint.h>
#include <libreg/load.h>

// Constants

#define REG_N_RST_COEFFS           10                           //!< RST order + 1 (must be \f$\leq\f$ #REG_RST_HISTORY_MASK)
#define REG_RST_HISTORY_MASK       31                           //!< History buffer index mask (must be \f$2^{N}-1\f$)
#define REG_MM_WARNING_THRESHOLD   0.4                          //!< #REG_WARNING level for Modulus Margin

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
    double                      r[REG_N_RST_COEFFS];            //!< R polynomial coefficients (measurement). See also #REG_N_RST_COEFFS.
    double                      s[REG_N_RST_COEFFS];            //!< S polynomial coefficients (actuation). See also #REG_N_RST_COEFFS.
    double                      t[REG_N_RST_COEFFS];            //!< T polynomial coefficients (reference). See also #REG_N_RST_COEFFS.
};

/*!
 * RST algorithm parameters
 */
struct reg_rst_pars
{
    enum reg_status             status;                         //!< Regulation parameters status
    enum reg_mode               reg_mode;                       //!< Regulation mode (#REG_CURRENT | #REG_FIELD)
    uint32_t                    alg_index;                      //!< Algorithm index (1-5). Based on pure delay
    uint32_t                    dead_beat;                      //!< 0 = not dead-beat, 1-3 = dead-beat (1-3)
    double                      reg_period;                     //!< Regulation period
    float                       inv_reg_period_iters;           //!< \f$\frac{1}{reg\_period\_iters}\f$
    float                       ref_advance;                    //!< Reference advance time
    float                       pure_delay_periods;             //!< Pure delay in regulation periods
    float                       track_delay_periods;            //!< Track delay in regulation periods
    float                       ref_delay_periods;              //!< Reference delay in regulation periods
    double                      inv_s0;                         //!< \f$\frac{1}{S[0]}\f$
    double                      t0_correction;                  //!< Correction to t[0] for rounding errors
    double                      inv_corrected_t0;               //!< \f$\frac{1}{T[0]+ t0\_correction}\f$
    struct reg_rst              rst;                            //!< RST polynomials
    double                      a   [REG_N_RST_COEFFS];         //!< Plant numerator A. See also #REG_N_RST_COEFFS.
    double                      b   [REG_N_RST_COEFFS];         //!< Plant denominator B. See also #REG_N_RST_COEFFS.
    double                      as  [REG_N_RST_COEFFS];         //!< \f$A \cdot S\f$. See also #REG_N_RST_COEFFS.
    double                      asbr[REG_N_RST_COEFFS];         //!< \f$A \cdot S + B \cdot R\f$. See also #REG_N_RST_COEFFS.
    int32_t                     jurys_result;                   //!< Jury's test result index (0 = OK)
    float                       min_auxpole_hz;                 //!< Minimum of RST auxpole*_hz parameters. Used to limit the scan frequency range.
    float                       modulus_margin;                 //!< Modulus margin. Equal to the minimum value of the sensitivity function (abs_S_p_y)
    float                       modulus_margin_freq;            //!< Frequency for modulus margin.
};

/*!
 * RST algorithm variables
 */
struct reg_rst_vars
{
    uint32_t                    history_index;                  //!< Index to latest entry in the history
    float                       prev_ref_rate;                  //!< Ref rate from previous iteration

    float                       openloop_ref[REG_RST_HISTORY_MASK+1]; //!< Openloop calculated reference history. See also #REG_RST_HISTORY_MASK.
    float                       ref         [REG_RST_HISTORY_MASK+1]; //!< RST calculated reference history. See also #REG_RST_HISTORY_MASK.
    float                       meas        [REG_RST_HISTORY_MASK+1]; //!< RST measurement history. See also #REG_RST_HISTORY_MASK.
    float                       act         [REG_RST_HISTORY_MASK+1]; //!< RST actuation history. See also #REG_RST_HISTORY_MASK.
};

// RST macro "functions"

#define regRstIncHistoryIndexRT(rst_vars_p) (rst_vars_p)->history_index = ((rst_vars_p)->history_index + 1) & REG_RST_HISTORY_MASK
#define regRstPrevRefRT(rst_vars_p) (rst_vars_p)->ref[(rst_vars_p)->history_index]
#define regRstDeltaRefRT(rst_vars_p) (regRstPrevRefRT(rst_vars_p) - (rst_vars_p)->ref[((rst_vars_p)->history_index - 1) & REG_RST_HISTORY_MASK])

// RST regulation functions

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Prepare coefficients for the RST regulation algorithm, to allow a pure loop delay to be accommodated.
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
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out] pars                   RST parameters object to update with the new coefficients.
 * @param[in]  reg_period_iters       Regulation period in iterations.
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
 * @param[in]  reg_mode               Regulation mode (voltage, current or field)
 * @param[in]  manual                 Pre-calculated RST parameters. These are used if auxpoles1_hz is zero.
 *
 * @retval     REG_OK on success
 * @retval     REG_WARNING if reg_rst_pars::modulus_margin < #REG_MM_WARNING_THRESHOLD
 * @retval     REG_FAULT if s[0] is too small (\f$<1\times 10^{-10}\f$) or is unstable (has poles outside the unit circle)
 */
enum reg_status regRstInit(struct reg_rst_pars *pars, uint32_t reg_period_iters, double reg_period,
                           struct reg_load_pars *load, float auxpole1_hz, float auxpoles2_hz,
                           float auxpoles2_z, float auxpole4_hz, float auxpole5_hz,
                           float pure_delay_periods, float track_delay_periods,
                           enum reg_mode reg_mode, struct reg_rst *manual);

/*!
 * Use the supplied RST parameters to calculate the actuation based on the supplied reference and measurement values.
 * If the actuation is clipped then regRstCalcRefRT() must be called to re-calculate the reference to put in the history.
 *
 * This is a Real-Time function.
 *
 * @param[in]     pars    RST coefficients and parameters
 * @param[in,out] vars    History of actuation, measurement and reference values. Updated with new values by this function.
 * @param[in]     ref     Latest reference value
 *
 * @returns       New actuation value
 */
float regRstCalcActRT(struct reg_rst_pars *pars, struct reg_rst_vars *vars, float ref);

/*!
 * Use the supplied RST parameters to back-calculate the reference based on the supplied actuation and measurement values.
 * This function must be called in two situations:
 * <ol>
 * <li>After calling regRstCalcActRT(), if the actuation has been clipped due to limits in the actuator.</li>
 * <li>If the system is running with open-loop actuation.</li>
 * </ol>
 * The function saves the new actuation in the RST history and back-calculates the reference, which is returned.
 *
 * This is a Real-Time function.
 *
 * @param[in]     pars    RST coefficients and parameters
 * @param[in,out] vars    History of actuation, measurement and reference values. Updated with new values by this function.
 * @param[in]     act     Latest actuation value
 *
 * @returns       Back-calculated reference value
 */
float regRstCalcRefRT(struct reg_rst_pars *pars, struct reg_rst_vars *vars, float act);

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

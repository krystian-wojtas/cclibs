/*!
 * @file  conv.h
 * @brief Converter Control Regulation library higher-level functions.
 *
 * The functions provided by conv.c combine all the elements needed to regulate current or field in the converter.
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

#ifndef LIBREG_CONV_H
#define LIBREG_CONV_H

// Include header files

#include <stdint.h>
#include <stdbool.h>

// Global power converter regulation constants

#define REG_N_LOADS                             4       //!< Number of loads addressed by LOAD SELECT
#define REG_INHIBIT_MAX_ABS_ERR_ITERATIONS      10      //!< Number of iterations to inhibit max_abs_err calculation

// Global power converter regulation structures

/*!
 * Reference function check structures
 */

struct reg_ref_check
{
    enum   reg_mode             reg_mode;               //!< Regulation mode. Can be #REG_VOLTAGE, #REG_CURRENT or #REG_FIELD.
    uint32_t                    num_samples;            //!< Number of reference samples
    float                       sum_ref_squared;        //!< Sum of ref^2
    float                       prev_ref;               //!< Previous reference
    float                       prev_v;                 //!< Previous estimated voltage
    struct reg_lim_ref          lim_v;                  //!< Voltage limits structure
};

/*!
 * Measurement simulation structure
 */
struct reg_conv_sim_meas
{
    struct reg_delay            meas_delay;             //!< Measurement delay parameters
    struct reg_noise_and_tone   noise_and_tone;         //!< Simulated noise and tone parameters
    float                       signal;                 //!< Simulated measured signal with noise and tone
};

/*!
 * RST parameters structure
 */
struct reg_conv_rst_pars
{
    bool                        use_next_pars;          //!< Flag to indicate whether to switch to the next set of RST parameters in the RT thread
    struct reg_rst_pars        *active;                 //!< Pointer to active parameters in pars[]
    struct reg_rst_pars        *next;                   //!< Pointer to next parameters in pars[]
    struct reg_rst_pars         pars[2];                //!< Structures for active and next RST parameter
};

/*!
 * Converter signal (field or current) regulation structure
 */
struct reg_conv_signal
{
    enum reg_enabled_disabled   regulation;             //!< Option to regulate this signal is enabled or disabled
    uint32_t                    iteration_counter;      //!< Iteration counter (within each regulation period)
    uint32_t                    reg_period_iters;       //!< Regulation period (in iterations) for Operational and Test parameters
    double                      reg_period;             //!< Regulation period(s) for Operational and Test parameters
    struct reg_meas_signal     *input_p;                //!< Pointer to input measurement signal structure
    struct reg_meas_signal      input;                  //!< Input measurement and measurement status
    uint32_t                    invalid_input_counter;  //!< Counter for invalid input measurements
    struct reg_meas_filter      meas;                   //!< Unfiltered and filtered measurement (real or sim)
    struct reg_meas_rate        rate;                   //!< Estimation of the rate of change of the measurement
    struct reg_lim_meas         lim_meas;               //!< Measurement limits
    struct reg_lim_ref          lim_ref;                //!< Reference limits
    struct reg_rst_pars        *rst_pars;               //!< Active RST parameters (Active Operational or Test)
    struct reg_rst_vars         rst_vars;               //!< RST regulation variables
    struct reg_conv_rst_pars    op_rst_pars;            //!< Operational regulation RST parameters
    struct reg_rst_pars         last_op_rst_pars;       //!< Last initialised operational RST parameters for debugging
    struct reg_conv_rst_pars    test_rst_pars;          //!< Test regulation RST parameters
    struct reg_rst_pars         last_test_rst_pars;     //!< Last initialised test RST parameters for debugging
    struct reg_err              err;                    //!< Regulation error
    struct reg_conv_sim_meas    sim;                    //!< Simulated measurement with noise and tone
};

/*!
 * Converter voltage structure
 */
struct reg_conv_voltage
{
    struct reg_meas_signal     *input_p;                //!< Pointer to input measurement signal structure
    struct reg_meas_signal      input;                  //!< Input measurement and measurement status
    uint32_t                    invalid_input_counter;  //!< Counter for invalid input measurements
    float                       meas;                   //!< Unfiltered voltage measurement (real or sim)
    struct reg_lim_ref          lim_ref;                //!< Voltage reference limits
    struct reg_err              err;                    //!< Voltage regulation error
    struct reg_conv_sim_meas    sim;                    //!< Simulated voltage measurement with noise and tone
    float                       ref;                    //!< Voltage reference before saturation or limits
    float                       ref_sat;                //!< Voltage reference after saturation compensation
    float                       ref_limited;            //!< Voltage reference after saturation and limits
};

/*!
 * Global converter regulation structure.
 */
struct reg_conv
{
    uint32_t                    iter_period_us;         //!< Iteration (measurement) period in microseconds
    double                      iter_period;            //!< Iteration (measurement) period in seconds

    // Libreg initialization parameter structures

    struct reg_pars             pars;                   //!< Libreg parameter structures
    struct reg_par_values       par_values;             //!< Private copy of all libreg parameter values

    // Regulation reference and measurement variables and parameters

    enum   reg_mode             reg_mode;               //!< Regulation mode. Can be #REG_NONE, #REG_VOLTAGE, #REG_CURRENT or #REG_FIELD.
    enum   reg_rst_source       reg_rst_source;         //!< RST parameter source. Can be #REG_OPERATIONAL_RST_PARS or #REG_TEST_RST_PARS.
    bool                        is_openloop;            //!< Open loop when true, closed loop when false
    struct reg_conv_signal     *reg_signal;             //!< Pointer to currently regulated signal structure. Can be reg_conv::b or reg_conv::i.
    struct reg_lim_ref         *lim_ref;                //!< Pointer to the currently active reference limit (b, i or v)
    struct reg_ref_check        ref_check;              //!< Reference function check data
    double                      reg_period;             //!< Regulation period
    float                       ref_advance;            //!< Time to advance reference function

    float                       meas;                   //!< Field or current regulated measurement
    float                       ref;                    //!< Field or current reference
    float                       ref_limited;            //!< Field or current reference after limits
    float                       ref_rst;                //!< Field or current reference after RST back-calculation
    float                       ref_openloop;           //!< Field or current reference after open loop back-calculation
    float                       ref_delayed;            //!< Field or current reference delayed by ref_delay_periods
    float                       track_delay_periods;    //!< Measured track_delay in regulation periods

    struct
    {
        uint32_t                ref_clip;               //!< Reference is being clipped
        uint32_t                ref_rate;               //!< Reference rate of change is being clipped
    } flags;                                            //!< Reference (field, current or voltage) limit flags

    // Field, current and voltage regulation structures

    struct reg_conv_signal      b;                      //!< Field regulation parameters and variables
    struct reg_conv_signal      i;                      //!< Current regulation parameters and variables
    struct reg_conv_voltage     v;                      //!< Voltage regulation parameters and variables. Voltage is regulated by voltage source.

    // Load parameters and variables structures

    struct reg_load_pars        load_pars;              //!< Circuit load model for regulation for LOAD SELECT
    struct reg_load_pars        load_pars_test;         //!< Circuit load model for regulation for LOAD TEST_SELECT

    struct reg_sim_vs_pars      sim_vs_pars;            //!< Voltage source simulation parameters
    struct reg_sim_load_pars    sim_load_pars;          //!< Circuit load model for simulation

    struct reg_sim_vs_vars      sim_vs_vars;            //!< Voltage source simulation variables
    struct reg_sim_load_vars    sim_load_vars;          //!< Load simulation variables

    // RMS current limits

    struct reg_lim_rms          lim_i_rms;              //!< Converter RMS current limits
    struct reg_lim_rms          lim_i_rms_load;         //!< Load RMS current limits
};

// Converter control functions

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Initialise the global converter regulation structure.
 * Set up internal pointers within the reg_conv struct and set the iteration period from the supplied parameter.
 * reg_conv::reg_mode is initialised to #REG_NONE and reg_conv::reg_rst_source is initialised to #REG_OPERATIONAL_RST_PARS.
 * The field_regulation and current_regulation parameters are used to enable or disable the option to regulate
 * current or field.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    conv               Pointer to converter regulation structure.
 * @param[in]     iter_period_us     Iteration (measurement) period in microseconds
 * @param[in]     field_regulation   Field regulation control (ENABLED/DISABLED)
 * @param[in]     current_regulation Current regulation control (ENABLED/DISABLED)
 */
void regConvInit(struct reg_conv *conv, uint32_t iter_period_us,
                 enum reg_enabled_disabled field_regulation, enum reg_enabled_disabled current_regulation);

/*!
 * Check libreg parameters for changes and run appropriate initialisation functions.
 * This should be called by the non-real-time thread of the application whenever any libreg parameters
 * have changed.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    conv           Pointer to converter regulation structure.
 * @param[in]     pars_mask      Parameter function mask (normally set to zero since the function will check
 *                               for parameters that have changed and will set the mask internally. Each set
 *                               bit will trigger the execution of the initialisation functions.
 */
void regConvPars(struct reg_conv *conv, uint32_t pars_mask);

/*!
 * Initialise a simulation with a given regulation mode and measurement of the associated signal.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in,out] conv                 Pointer to converter regulation structure.
 * @param[in]     reg_mode             Initial regulation mode.
 * @param[in]     start                Initial value for signal identified by reg_mode.
 */
void regConvSimInit(struct reg_conv *conv, enum reg_mode reg_mode, float start);

/*!
 * Initialise a converter structure with voltage, current and field measurement signal structures.
 * The supplied pointer values are copied into reg_conv::v, reg_conv::i and reg_conv::b. If NULL
 * values are supplied, the appropriate pointer is set to point to a statically-allocated object
 * with reg_meas_signal::signal = 0.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    conv        Pointer to converter regulation structure.
 * @param[in]     v_meas_p    Pointer to voltage input measurement signal structure. NULL is allowed.
 * @param[in]     i_meas_p    Pointer to current input measurement signal structure. NULL is allowed.
 * @param[in]     b_meas_p    Pointer to field input measurement signal structure. NULL is allowed.
 */
void regConvMeasInit(struct reg_conv *conv, struct reg_meas_signal *v_meas_p, struct reg_meas_signal *i_meas_p, struct reg_meas_signal *b_meas_p);

/*!
 * Initialise reference function check
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in,out] conv                 Pointer to the converter regulation structure.
 * @param[in]     reg_mode             #REG_CURRENT or #REG_FIELD for reference function to be checked.
 * @param[in]     init_ref             Initial reference (field or current).
 * @param[in]     invert_limits        #REG_ENABLED if voltage limits should be inverterd, otherwise #REG_DISABLED.
 *
 * @returns   true if es
 */
void regConvRefCheckInit(struct reg_conv *conv, enum reg_mode reg_mode, float init_ref, enum reg_enabled_disabled invert_limits);

/*!
 * Initialise reference function check
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in,out] conv                 Pointer to the converter regulation structure.
 * @param[in]     reg_mode             #REG_CURRENT or #REG_FIELD for reference function to be checked.
 * @param[in]     init_ref             Initial reference (field or current).
 * @param[in]     invert_limits        #REG_ENABLED if voltage limits should be inverterd, otherwise #REG_DISABLED.
 *
 * @returns   true if es
 */
bool regConvRefCheck(struct reg_conv *conv, float ref, float *v_min_limit, float *v_max_limit,
                     float *estimated_v, float *estimated_i);

/*!
 * Initialise reference function check
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in]     conv                 Pointer to the converter regulation structure.
 *
 * @returns   Root mean squared for the reference function that was just checked using
 */
float regConvRefCheckRms(struct reg_conv *conv);


/*!
 * Set converter regulation mode.
 * For current and field: if reg_conv_signal::op_rst_pars or reg_conv_signal::test_rst_pars
 * has flag reg_conv_rst_pars::use_next_pars set, switch in the new set of RST parameters,
 * specified in reg_conv_rst_pars::next. Then unset the flag.
 *
 * If the regulation mode changes when the new parameters are switched in, <em>conv</em> is updated
 * depending on the new regulation mode. In the case of #REG_NONE, the voltage reference values in
 * reg_conv::v are reset to zero. In the case of #REG_VOLTAGE, the voltage references are set
 * according to the previous regulation mode. In the case of #REG_CURRENT or #REG_FIELD, then
 * regulation will be open-loop or closed-loop depending on the actuation type. If actuation
 * is #REG_CURRENT_REF, it is open-loop and the voltage reference is reset to zero. If actuation
 * is #REG_VOLTAGE_REF, regulation is closed-loop.
 *
 * If we are closing loop on current, the voltage reference is adjusted for magnet saturation,
 * assuming that the current is invariant. This assumes it is unlikely that the current regulation
 * will start with the current ramping fast while deep into the magnet saturation zone.
 *
 * Finally, this function resets the maximum absolute error for field and current, even if the mode
 * does not change.
 *
 * This is a Real-Time function. Note that reading and unsetting the flags is not an atomic
 * operation, so it is assumed that this function will be called from one thread only.
 *
 * @param[in,out] conv                 Pointer to converter regulation structure.
 * @param[in]     reg_mode             Regulation mode to set (#REG_NONE, #REG_VOLTAGE, #REG_CURRENT or #REG_FIELD).
 */
void regConvModeSetRT(struct reg_conv *conv, enum reg_mode reg_mode);

/*!
 * Start of real-time processing on each iteration.
 * Receive new voltage, current and field measurements, then apply limits and filters.
 *
 * First, check reg_conv::b and reg_conv::i and swap RST parameter pointers for field and current if
 * reg_conv_rst_pars::use_next_pars flag is set. Select simulated or real measurements based on
 * input parameter <em>use_sim_meas</em>. For field and current regulation, update the iteration
 * counter, resetting it to zero if the counter has reached the end of the regulation period
 * (= reg_rst_pars::reg_period_iters).
 *
 * Next, check the voltage, current and field measurements. If the voltage measurement is invalid,
 * use the voltage source model instead (reg_err::delayed_ref).
 *
 * If current is being regulated, use the delayed reference (reg_conv::ref_delayed) adjusted by
 * the regulation error (reg_err::err). Otherwise, extrapolate the previous value using the current
 * rate of change (reg_meas_rate::estimate). Field measurements are treated in the same way.
 *
 * Next, check the current measurement limits with a call to regLimMeasRT(). If field is being
 * regulated, field measurement limits are also checked. If actuation is #REG_VOLTAGE_REF, the
 * voltage regulation and reference limits are checked with regErrCheckLimitsRT() and regLimVrefCalcRT().
 * Note that voltage limits can depend on the current, so voltage reference limits are calculated
 * with respect to reg_meas_filter::signal[#REG_MEAS_FILTERED].
 *
 * Finally, the field and current measurements are filtered with regMeasFilterRT() and the measurement
 * rates are estimated with regMeasRateRT().
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in,out] conv                 Pointer to the converter regulation structure.
 * @param[in]     reg_rst_source       #REG_OPERATIONAL_RST_PARS or #REG_TEST_RST_PARS
 * @param[in]     unix_time            Unix time for this iteration
 * @param[in]     us_time              Microsecond time for this iteration
 * @param[in]     use_sim_meas         If zero, use the real field, current and voltage measurements
 *                                     and measurement statuses supplied by the application. Otherwise
 *                                     use simulated measurements. If simulated measurements are used,
 *                                     all instances of reg_meas_signal::status are set to #REG_MEAS_SIGNAL_OK.
 *
 * @returns Iteration number (0 indicates that the reference should be calculated on this iteration)
 */
uint32_t regConvMeasSetRT(struct reg_conv *conv, enum reg_rst_source reg_rst_source,
                          uint32_t unix_time, uint32_t us_time, uint32_t use_sim_meas);

/*!
 * Regulate voltage, current or field for one regulation period.
 *
 * If the regulation mode is #REG_VOLTAGE (the reference is a voltage reference), the function applies
 * limits and updates the RST actuation history for field and current, to prepare for a smooth switch
 * to another regulation mode.
 *
 * If the regulation mode is #REG_CURRENT or #REG_FIELD, the function checks whether the measurement is
 * above the threshold for closed loop regulation. This determines whether to use the RST or open loop
 * algorithm for regulation. In both cases, the function applies the current reference clip and rate limits
 * with regLimRefRT() and keeps the RST and open loop actuation histories up-to-date for both regulation modes
 * to allow for mode switching.
 *
 * If the actuation is #REG_CURRENT_REF, then control is open loop. In this case, just store the limited reference.
 *
 * Otherwise, the regulation is #REG_CURRENT or #REG_FIELD and the actuation is #REG_VOLTAGE_REF. In this
 * case, we execute the following steps for each regulation period:
 *
 * <ol>
 * <li>Calculate the voltage reference using the RST algorithm, regRstCalcActRT().</li>
 * <li>[If we are regulating current] Calculate the magnet saturation compensation with regLoadVrefSatRT().</li>
 * <li>Apply voltage reference clip and rate limits with regLimRefRT().</li>
 * <li>If the voltage reference has been clipped:
 * <ul>
 *     <li>[If we are regulating current] Back-calculate the new voltage reference before the saturation
 *     compensation, using regLoadInverseVrefSatRT().</li>
 *     <li>Mark the current reference as rate-limited (reg_lim_ref::rate).</li>
 * </ul>
 * <li>Back-calculate new current reference to keep RST and open loop histories balanced, using regRstCalcRefRT().</li>
 * </ol>
 *
 * Finally, in all cases, this function monitors the regulation error using the delayed reference and the
 * filtered measurement with regErrCheckLimitsRT().
 *
 * This is a Real-Time function.
 *
 * @param[in,out] conv                 Pointer to converter regulation structure.
 * @param[in]     ref                  Pointer to reference (voltage, current or field according to conv::reg_mode).
 */
void regConvRegulateRT(struct reg_conv *conv, float *ref);

/*!
 * Simulate the voltage source and load and the measurements of the voltage, current and field.
 *
 * If the actuation is #REG_VOLTAGE_REF, the voltage source response to the reference voltage is simulated with
 * regSimVsRT(), without taking into account V_REF_DELAY (see reg_sim_load_vars). The voltage reference comes from
 * reg_conv_voltage::ref_limited. The load current and field are simulated with regSimLoadRT() in response to the
 * voltage source response plus the perturbation (specified as <em>v_perturbation</em>).
 *
 * If the actuation is #REG_CURRENT_REF, we use the voltage source model regSimVsRT() as the current source model and
 * assume that all the circuit current passes through the magnet. <em>i.e.</em>, this assumes that the parallel resistance
 * \f$R_{p}\f$ is large. If this is not true then the simulation will not be accurate. Next, we derive the circuit voltage
 * using \f$V = I\cdot R + L(I)\cdot\frac{dI}{dt}\f$. \f$\frac{dI}{dt}\f$ is calculated from reg_sim_load_vars::magnet_current,
 * which contains the current from the previous iteration. Finally, we derive the simulated magnetic field with regLoadCurrentToFieldRT().
 *
 * After simulating the voltage or current source model, we use delays to estimate the measurement of the magnet's field and the
 * circuit's current and voltage, with regDelaySignalRT().
 *
 * The simulated voltage measurement without noise is used as the delayed reference for the voltage error calculation and is stored
 * in reg_conv_sim_meas::signal.
 *
 * Finally, we simulate noise and tone on the simulated measurement of the magnet's field and the circuit's current and voltage,
 * using regMeasNoiseAndToneRT().
 *
 * This is a Real-Time function.
 *
 * @param[in,out] conv                 Pointer to converter regulation structure.
 * @param[in]     v_perturbation       Voltage perturbation to add to the simulated circuit voltage.
 */
void regConvSimulateRT(struct reg_conv *conv, float v_perturbation);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_CONV_H

// EOF

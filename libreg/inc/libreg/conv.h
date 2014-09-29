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

/*!
 * Regulation parameters actuation (voltage or current)
 */
enum reg_actuation
{
    REG_VOLTAGE_REF,                                    //!< Actuation is a voltage reference
    REG_CURRENT_REF                                     //!< Actuation is a current reference
};

/*!
 * Regulation parameters source (operational or test)
 */
enum reg_rst_source
{
    REG_OPERATIONAL_RST_PARS,                           //!< Use operational RST parameters
    REG_TEST_RST_PARS                                   //!< Use test RST parameters
};

/*!
 * Regulation error rate control
 */
enum reg_err_rate
{
    REG_ERR_RATE_REGULATION,                            //!< Calculate regulation error at regulation rate
    REG_ERR_RATE_MEASUREMENT                            //!< Calculate regulation error at measurement rate
};

// Global power converter regulation structures

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
    uint32_t                    use_next_pars;          //!< Signal to use next RST pars in the RT thread
    struct reg_rst_pars        *active;                 //!< Pointer to active parameters in pars[]
    struct reg_rst_pars        *next;                   //!< Pointer to next parameters in pars[]
    struct reg_rst_pars        *debug;                  //!< Pointer to most recently initialized parameters in pars[]
    struct reg_rst_pars         pars[2];                //!< Structures for active and next RST parameter
};

/*!
 * Converter signal (field or current) regulation structure
 */
struct reg_conv_signal
{
    struct reg_meas_signal     *input_p;                //!< Pointer to input measurement signal structure
    struct reg_meas_signal      input;                  //!< Input measurement and measurement status
    uint32_t                    invalid_input_counter;  //!< Counter for invalid input measurements
    struct reg_meas_filter      meas;                   //!< Unfiltered and filtered measurement (real or sim)
    struct reg_meas_rate        rate;                   //!< Estimation of the rate of the field measurement
    struct reg_lim_meas         lim_meas;               //!< Measurement limits
    struct reg_lim_ref          lim_ref;                //!< Reference limits
    struct reg_rst_pars        *rst_pars;               //!< Active RST parameters (Active Operational or Test)
    struct reg_conv_rst_pars    op_rst_pars;            //!< Operational regulation RST parameters
    struct reg_conv_rst_pars    test_rst_pars;          //!< Test regulation RST parameters
    enum   reg_err_rate         err_rate;               //!< Rate control for regulation error calculation
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
    struct reg_rst_pars         rst_pars;               //!< Regulation RST parameters
    enum   reg_err_rate         err_rate;               //!< Rate control for regulation error calculation
    struct reg_err              err;                    //!< Voltage regulation error
    struct reg_conv_sim_meas    sim;                    //!< Simulated voltage measurement with noise and tone
    float                       ref;                    //!< Voltage reference before saturation or limits
    float                       ref_sat;                //!< Voltage reference after saturation compensation
    float                       ref_limited;            //!< Voltage reference after saturation and limits
};

/*!
 * Global converter regulation structure.
 * The regulation reference and measurement variables and parameters are defined by { actuation, ..., rst_vars }.
 * { b, i, v } are the field, current and voltage regulation structures. The load parameters and variables are
 * defined by { load_pars, ..., sim_load_vars }.
 */
struct reg_conv
{
    double                      iter_period;            //!< Iteration (measurement) period

    // Regulation reference and measurement variables and parameters

    enum   reg_actuation        actuation;              //!< Converter actuation. Can be #REG_VOLTAGE_REF or #REG_CURRENT_REF.
    enum   reg_mode             reg_mode;               //!< Regulation mode. Can be #REG_VOLTAGE, #REG_CURRENT or #REG_FIELD.
    enum   reg_rst_source       reg_rst_source;         //!< RST parameter source. Can be #REG_OPERATIONAL_RST_PARS or #REG_TEST_RST_PARS.
    struct reg_conv_signal     *reg_signal;             //!< Pointer to currently regulated signal structure. Can be reg_conv::b or reg_conv::i.

    uint32_t                    iteration_counter;      //!< Iteration counter (within each regulation period)
    double                      reg_period;             //!< Regulation period
    float                       ref_advance;            //!< Time to advance reference function

    float                       meas;                   //!< Field or current regulated measurement
    float                       ref;                    //!< Field or current reference
    float                       ref_limited;            //!< Field or current reference after limits
    float                       ref_rst;                //!< Field or current reference after back-calculation
    float                       ref_delayed;            //!< Field or current reference delayed by track_delay

    struct
    {
        uint32_t                ref_clip;               //!< Reference is being clipped
        uint32_t                ref_rate;               //!< Reference rate of change is being clipped
    } flags;                                            //!< Reference (field, current or voltage) limit flags

    struct reg_rst_vars         rst_vars;               //!< Field or current regulation RST variables

    // Field, current and voltage regulation structures

    struct reg_conv_signal      b;                      //!< Field regulation parameters and variables
    struct reg_conv_signal      i;                      //!< Current regulation parameters and variables
    struct reg_conv_voltage     v;                      //!< Voltage regulation parameters and variables. Voltage is regulated by voltage source.

    // Load parameters and variables structures

    struct reg_load_pars        load_pars;              //!< Circuit load model for regulation

    struct reg_sim_vs_pars      sim_vs_pars;            //!< Voltage source simulation parameters
    struct reg_sim_load_pars    sim_load_pars;          //!< Circuit load model for simulation

    struct reg_sim_vs_vars      sim_vs_vars;            //!< Voltage source simulation variables
    struct reg_sim_load_vars    sim_load_vars;          //!< Load simulation variables
};

// Converter control macro "functions"

#define regConvInitRefGenFunc(reg_p, reg_ref_gen_function) (reg_p)->reg_ref_gen_function=reg_ref_gen_function

// Converter control functions

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Initialise the global converter regulation structure.
 * Set up internal pointers within the reg_conv struct and set the iteration period and actuation mode from the supplied parameters.
 * reg_conv::reg_mode is initialised to #REG_NONE and reg_conv::reg_rst_source is initialised to #REG_OPERATIONAL_RST_PARS.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    conv           Converter regulation structure to initialise
 * @param[in]     iter_period    Iteration (measurement) period
 * @param[in]     actuation      Converter actuation (#REG_VOLTAGE_REF | #REG_CURRENT_REF).
 */
void regConvInit(struct reg_conv *conv, double iter_period, enum reg_actuation actuation);

/*!
 * Initialise RST regulation for the specified scenario.
 * Regulation can be field or current and parameters can be operational or test. If we are using voltage actuation,
 * the RST parameters are validated and used to initialise RST regulation by a call to regRstInit(). If we are
 * using current actuation, RST regulation is not initialised; instead the function prepares the periods in
 * reg_conv_rst_pars::next, so that the delayed_reg calculation works.
 *
 * This function maintains a flag reg_conv_rst_pars::use_next_pars. If the flag is unset (== 0), then the function
 * behaves as described above, sets the flag and returns 1. If the flag is set (== 1), then the function is waiting
 * for the real-time thread to reset the flag, so it does nothing and returns 0.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in,out] conv                   RST parameters. The subset of parameters to be used is specified using
 *                                       <em>reg_mode</em> and <em>reg_rst_source</em> (see below). If the actuation
 *                                       is #REG_VOLTAGE_REF, the RST regulation will be inititialised by a call to
 *                                       regRstInit() (see below). If the actuation is #REG_CURRENT_REF, RST regulation
 *                                       is not initialised, but the periods are prepared so that the delayed_reg
 *                                       calculation works.
 * @param[in]     reg_signal             Used to specify filter parameters. reg_conv_signal::meas is used in the
 *                                       calculation of <em>pure_delay_periods</em> (see below).
 * @param[in]     reg_mode               Regulation mode. If set to #REG_CURRENT, use parameters from reg_conv::i.
 *                                       If set to #REG_FIELD, use parameters from reg_conv::b.
 * @param[in]     reg_rst_source         Select operation or test parameters. If set to #REG_OPERATIONAL_RST_PARS,
 *                                       use parameters from reg_conv_signal::op_rst_pars, otherwise use parameters
 *                                       from reg_conv_signal::test_rst_pars.
 * @param[in]     reg_period_iters       Regulation period. Specified as an integer number of iteration periods,
 *                                       as regulation can only run on iteration boundaries. Passed as a parameter
 *                                       to regRstInit().
 * @param[in]     auxpole1_hz            Frequency of (real) auxillary pole 1. If auxpole1_hz \f$\leq 0\f$, the I,
 *                                       PI and PII controllers are not used to calculate the RST coefficients;
 *                                       instead, manually-calculated coefficients must be supplied (see the
 *                                       <em>manual_[r,s,t]</em> parameters below). Passed as a parameter to
 *                                       regRstInit().
 * @param[in]     auxpoles2_hz           Frequency of (conjugate) auxillary poles 2 and 3. Passed as a parameter
 *                                       to regRstInit().
 * @param[in]     auxpoles2_z            Damping of (conjugate) auxillary poles 2 and 3. Passed as a parameter to
 *                                       regRstInit().
 * @param[in]     auxpole4_hz            Frequency of (real) auxillary pole 4. Passed as a parameter to regRstInit().
 * @param[in]     auxpole5_hz            Frequency of (real) auxillary pole 5. Passed as a parameter to regRstInit().
 * @param[in]     pure_delay_periods     Pure delay in the regulation loop, specified as number of periods. This
 *                                       models the voltage reference delay + measurement delay + voltage source
 *                                       response) as a simple delay. For stability, this cannot be more than 40%
 *                                       of the regulation period. If this parameter is set to zero, it will be
 *                                       calculated. Passed as a parameter to regRstInit().
 * @param[in]     track_delay_periods    Anticipated delay between the setting of the field/current reference and
 *                                       the moment when the measurement should equal the reference. Specified as
 *                                       a number of iterations. The track delay depends upon the type of
 *                                       regulator being used; set to 1 in the case of the dead-beat PII algorithm.
 *                                       Passed as a parameter to regRstInit().
 * @param[in]     manual_r               Pre-calculated R coefficients for the RST algorithm. See also
 *                                       #REG_N_RST_COEFFS. Packed into a reg_rst struct and passed to regRstInit().
 * @param[in]     manual_s               Pre-calculated S coefficients for the RST algorithm. See also
 *                                       #REG_N_RST_COEFFS. Packed into a reg_rst struct and passed to regRstInit().
 * @param[in]     manual_t               Pre-calculated T coefficients for the RST algorithm. See also
 *                                       #REG_N_RST_COEFFS. Packed into a reg_rst struct and passed to regRstInit().
 *
 * @returns       0 if reg_conv_rst_pars::use_next_pars == 1 (set by a previous call to this function)
 * @returns       1 if reg_conv_rst_pars::use_next_pars == 0
 */
uint32_t regConvRstInit(struct reg_conv *conv, struct reg_conv_signal *reg_signal, enum reg_mode reg_mode,
                        enum reg_rst_source reg_rst_source, uint32_t reg_period_iters, float auxpole1_hz,
                        float auxpoles2_hz, float auxpoles2_z, float auxpole4_hz, float auxpole5_hz,
                        float pure_delay_periods, float track_delay_periods, double manual_r[REG_N_RST_COEFFS],
                        double manual_s[REG_N_RST_COEFFS], double manual_t[REG_N_RST_COEFFS]);

/*!
 * Initialise the simulated load structures with the specified load parameters.
 * Initialises reg_conv::sim_load_pars with a call to regSimLoadTcError(). Then calls regSimLoadInit() and
 * regSimLoadSetVoltage(). Finally, sets the simulated measurement values for voltage (reg_conv::v),
 * current (reg_conv::i) and field (reg_conv::b).
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in,out] conv                 Converter regulation object to update. Input parameters are provided in
 *                                     reg_conv::load_pars.
 * @param[in]     reg_mode             Unused.
 * @param[in]     sim_load_tc_error    Simulation load time constant error. The load parameters will be distorted to
 *                                     have the required Tc error. If zero, the load parameters are used as supplied.
 */
void regConvInitSimLoad(struct reg_conv *conv, enum reg_mode reg_mode, float sim_load_tc_error);

/*!
 * Initialise a converter structure with voltage, current and field measurement signal structures.
 * The supplied pointer values are copied into reg_conv::v, reg_conv::i and reg_conv::b. If NULL
 * values are supplied, the appropriate pointer is set to point to a statically-allocated object
 * with reg_meas_signal::signal = 0.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    conv        Converter regulation object to update.
 * @param[in]     v_meas_p    Pointer to voltage input measurement signal structure. NULL is allowed.
 * @param[in]     i_meas_p    Pointer to current input measurement signal structure. NULL is allowed.
 * @param[in]     b_meas_p    Pointer to field input measurement signal structure. NULL is allowed.
 */
void regConvInitMeas(struct reg_conv *conv, struct reg_meas_signal *v_meas_p, struct reg_meas_signal *i_meas_p, struct reg_meas_signal *b_meas_p);

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
 * @param[in,out] conv                 Converter regulation object to update.
 * @param[in]     reg_mode             Regulation mode to set (None, Voltage, Current or Field).
 * @param[in]     reg_rst_source       Unused.
 * @param[in]     iteration_counter    In case of current or field regulation, with voltage actuation,
 *                                     this value is passed into reg_conv::iteration_counter.
 */
void regConvSetModeRT(struct reg_conv *conv, enum reg_mode reg_mode, enum reg_rst_source reg_rst_source, uint32_t iteration_counter);

/*!
 * Read voltage, current and field measurements, then apply limits and filters.
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
 * @param[in,out] conv                 Converter regulation object to update.
 * @param[in]     use_sim_meas         If zero, use the real field, current and voltage measurements
 *                                     and measurement statuses supplied by the application. Otherwise
 *                                     use simulated measurements. If simulated measurements are used,
 *                                     all instances of reg_meas_signal::status are set to #REG_MEAS_SIGNAL_OK.
 *
 * @returns Iteration number reg_conv::iteration_counter
 */
uint32_t regConvSetMeasRT(struct reg_conv *conv, uint32_t use_sim_meas);

/*!
 * Regulate voltage, current or field for one regulation period.
 *
 * If the actuation is #REG_CURRENT_REF, then control is open loop. In this case, just apply the current
 * reference limits. reg_conv_voltage::ref_limited is stored in the RST history reg_conv::rst_vars to allow
 * delayed reference calculation. reg_conv::meas is stored in the RST history to allow delay to be
 * removed when logging.
 *
 * Otherwise, if the actuation is #REG_VOLTAGE_REF, control is closed loop and depends on the regulation
 * mode. If we are regulating the voltage, all we need to do is apply the limits. If we are regulating
 * current or field, we execute the following steps for each regulation period:
 *
 * <ol>
 * <li>Apply the current reference clip and rate limits with regLimRefRT().</li>
 * <li>Calculate the voltage reference using the RST algorithm, regRstCalcActRT().</li>
 * <li>[If we are regulating current] Calculate the magnet saturation compensation with regLoadVrefSatRT().</li>
 * <li>Apply voltage reference clip and rate limits with regLimRefRT().</li>
 * <li>If the voltage reference has been clipped:
 * <ul>
 *     <li>[If we are regulating current] Back-calculate the new voltage reference before the saturation
 *     compensation, using regLoadInverseVrefSatRT().</li>
 *     <li>Back-calculate new current reference to keep RST histories balanced, using regRstCalcRefRT().</li>
 *     <li>Mark the current reference as rate-limited (reg_lim_ref::rate).</li>
 * </ul>
 * </ol>
 *
 * Finally, monitor the regulation error using the delayed reference and the filtered measurement with regErrCheckLimitsRT().
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in,out] conv                 Converter regulation object to update.
 * @param[in]     ref                  Reference for voltage, current or field.
 * @param[in]     enable_max_abs_err   if set to zero, reg_err::max_abs_err is zeroed. Otherwise calculate reg_err::max_abs_err.
 *
 * @retval 0 if reg_conv::iteration_counter != 0
 * @retval 1 if reg_conv::iteration_counter == 0
 */
uint32_t regConvRegulateRT(struct reg_conv *conv, float *ref, uint32_t enable_max_abs_err);

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
 * with regMeasNoiseAndToneRT().
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in,out] conv                 Converter regulation object to update.
 * @param[in]     v_perturbation       Voltage perturbation to add to the simulated circuit voltage.
 */
void regConvSimulateRT(struct reg_conv *conv, float v_perturbation);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_CONV_H

// EOF

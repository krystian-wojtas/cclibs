/*!
 * @file  sim.h
 * @brief Converter Control Regulation library power converter and load simulation functions
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

#ifndef LIBREG_SIM_H
#define LIBREG_SIM_H

#include <stdint.h>
#include <stdbool.h>
#include <libreg/load.h>

// Constants

#define REG_NUM_PC_SIM_COEFFS                   4               //!< Number of power converter (voltage or current source) simulation coefficients
#define REG_PC_SIM_UNDERSAMPLED_THRESHOLD       0.25            //!< Threshold for calculated power converter delay in iteration periods

// Simulation structures

/*!
 * Power converter simulation parameters
 *
 * Libreg supports the control of either a current source (PC ACTUATION is CURRENT_REF) and a 
 * voltage source (PC ACTUATION is VOLTAGE REF). The same power converter simulation model is
 * used in either case with the response being either the voltage applied to the 
 * circuit, or the current driven through the circuit. Libreg can use the Tustin algorithm
 * to calculate the z-coefficients for a second order model, or the application can supply the
 * coefficient for (up to) a third order model.
 *
 * PC ACT_DELAY_ITERS defines the delay between the start of an iteration in which the actuation 
 * (voltage or current reference) is calculated and the time that it enters the simulation of the voltage
 * or current source. This models the delay that might be due to computation time, DAC signal filtering, 
 * or a digital link between the controller running libreg and the power converter electronics.
 * Both the power converter and load models disregard this delay and execute as if it were zero.
 * The delay is then added to the simulated measurement of the simulated signals.
 */
struct reg_sim_pc_pars
{
    float                       num[REG_NUM_PC_SIM_COEFFS];     //!< Numerator coefficients b0, b1, b2, etc. See also #REG_NUM_PC_SIM_COEFFS.
    float                       den[REG_NUM_PC_SIM_COEFFS];     //!< Denominator coefficients a0, a2, a2, etc.
    float                       act_delay_iters;                //!< Delay before the voltage/current reference is applied to the voltage/current source.
    float                       rsp_delay_iters;                //!< Power converter response delay for steady actuation ramp.
    float                       gain;                           //!< \f[gain = \frac{\sum den}{\sum num}\f].
    bool                        is_pc_undersampled;             //!< Simulated power converter is under-sampled flag.
};

/*!
 * Power converter simulation variables
 */
struct reg_sim_pc_vars
{
    float                       act[REG_NUM_PC_SIM_COEFFS];     //!< Actuation history.
    float                       rsp[REG_NUM_PC_SIM_COEFFS];     //!< Voltage/current source response history ignoring PC ACT_DELAY_ITERS.
};

/*!
 * Load simulation parameters
 */
struct reg_sim_load_pars
{
    float                       tc_error;                       //!< Simulated load time constant error
    float                       period_tc_ratio;                //!< Simulation period / load time constant
    bool                        is_load_undersampled;           //!< Simulated load is under-sampled flag
    struct reg_load_pars        load_pars;                      //!< Simulated load parameters
};

/*!
 * Load simulation variables
 *
 * PC ACT_DELAY_ITERS defines the delay between the start of an iteration in which the actuation 
 * (voltage or current reference) is calculated and the time that it enters the simulation of the voltage
 * or current source. This models the delay that might be due to computation time, DAC signal filtering, 
 * or a digital link between the controller running libreg and the power converter electronics.
 */
struct reg_sim_load_vars
{
    float                       circuit_voltage;                //!< Circuit voltage (without PC ACT_DELAY_ITERS).
    float                       circuit_current;                //!< Circuit current (without PC ACT_DELAY_ITERS).
    float                       magnet_current;                 //!< Magnet current  (without PC ACT_DELAY_ITERS).
    float                       magnet_field;                   //!< Magnet field    (without PC ACT_DELAY_ITERS).
    float volatile              integrator;                     //!< Integrator for simulated current.
    float                       compensation;                   //!< Compensation for Kahan Summation.
};

#ifdef __cplusplus
extern "C" {
#endif

// Simulation functions

/*!
 * Initialise power converter (voltage source or current source) model. 
 * This function sets or clears reg_sim_pc_pars::is_pc_undersampled flag.
 *
 * This is a background function: do not call from the real-time thread or interrupt.
 *
 * @param[in,out] pars                 Pointer to power converter simulation parameters.
 * @param[in]     iter_period          Simulation iteration period in seconds.
 * @param[in]     act_delay_iters      Delay before the actuation is applied to the power converter.
 * @param[in]     bandwidth            Second order model bandwidth (-3 dB). Set to zero to use num,den.
 * @param[in]     z                    Second order model damping.
 * @param[in]     tau_zero             Second order zero time constant. Set to 0 if not required.
 * @param[in]     num                  Third order model numerator coefficients (used if bandwidth is zero).
 * @param[in]     den                  Third order model denominator coefficients (used if bandwidth is zero).
 */
void regSimPcInit(struct reg_sim_pc_pars *pars, float iter_period, float act_delay_iters,
                  float bandwidth, float z, float tau_zero,
                  float num[REG_NUM_PC_SIM_COEFFS], float den[REG_NUM_PC_SIM_COEFFS]);



/*!
 * Initialise the power converter simulation history to be in steady-state with the given initial response.
 *
 * <strong>Note:</strong> the model's gain must be calculated (using regSimPcInit()) before calling this function.
 *
 * This is a background function: do not call from the real-time thread or interrupt.
 *
 * @param[in]     pars                 Pointer to power converter simulation parameters.
 * @param[out]    vars                 Pointer to power converter simulation variables.
 * @param[in]     init_rsp             Initial power converter model response.
 * @returns       Steady-state actuation that will produce the supplied response.
 */
float regSimPcInitHistory(struct reg_sim_pc_pars *pars, struct reg_sim_pc_vars *vars, float init_rsp);



/*!
 * Initialise the load simulation parameters structure.
 *
 * This is a background function: do not call from the real-time thread or interrupt.
 *
 * @param[out]    sim_load_pars        Load simulation parameters object to update
 * @param[in]     load_pars            Load parameters
 * @param[in]     sim_load_tc_error    Simulation load time constant error. If the Tc
 *                                     error is zero, the simulation will simply use load_pars.
 *                                     Otherwise it will initialise the simulated load using
 *                                     distorted load_pars so that the simulated load time
 *                                     constant will mismatch the load_pars time constants by
 *                                     the required factor (i.e. 0.1 = 10% error in Tc).
 * @param[in]     sim_period           Simulation period
 */
void regSimLoadInit(struct reg_sim_load_pars *sim_load_pars, struct reg_load_pars *load_pars, float sim_load_tc_error, float sim_period);



/*!
 * Initialises the load simulation with the field b_init.
 *
 * This is a background function: do not call from the real-time thread or interrupt.
 *
 * @param[in]     pars                 Load simulation parameters
 * @param[in,out] vars                 Load simulation values object to update
 * @param[in]     b_init               Initial field value
 */
void regSimLoadSetField(struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float b_init);



/*!
 * Initialise the load simulation with the current
 *
 * This is a background function: do not call from the real-time thread or interrupt.
 *
 * @param[in]     pars                 Load simulation parameters
 * @param[in,out] vars                 Load simulation values object to update
 * @param[in]     i_init               Initial current value
 */
void regSimLoadSetCurrent(struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float i_init);



/*!
 * Initialise the load simulation with the load voltage
 *
 * This is a background function: do not call from the real-time thread or interrupt.
 *
 * @param[in]     pars                 Load simulation parameters
 * @param[in,out] vars                 Load simulation values object to update
 * @param[in]     v_init               Initial load voltage value
 */
void regSimLoadSetVoltage(struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float v_init);



/*!
 * Simulate the power converter response to the specified actuation.
 *
 * This is a Real-Time function.
 *
 * @param[in]     pars                 Pointer to power converter simulation parameters.
 * @param[in,out] vars                 Pointer to power converter simulation variables.
 * @param[in]     act                  Actuation (voltage or current reference).
 * @returns       Load voltage or current (according to PC ACTUATION)
 */
float regSimPcRT(struct reg_sim_pc_pars *pars, struct reg_sim_pc_vars *vars, float act);



/*!
 * Simulate the current in the load in response to the specified load voltage. The algorithm
 * is slightly different if the voltage source simulation and the load are under-sampled.
 * 
 * The computation of the reg_sim_load_vars::integrator makes use of the Kahan Summation
 * Algorithm, which largely improves the precision on the sum, especially in that specific
 * function where the increment is often very small compared to the integrated sum.
 * 
 * This is a Real-Time function (thread safe).
 *
 * @param[in]     pars                 Load simulation parameters.
 * @param[in,out] vars                 Load simulation values object to update.
 * @param[in]     is_pc_undersampled   Voltage Source undersampled flag. If false, use first-order interpolation
 *                                     of voltage. If true, voltage source is undersampled: use final
 *                                     voltage for complete sample.
 * @param[in]     v_circuit            Load voltage, stored in reg_sim_load_vars::circuit_voltage for the next
 *                                     iteration.
 * @returns       Circuit current
 */
float regSimLoadRT(struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, bool is_pc_undersampled, float v_circuit);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_SIM_H

// EOF

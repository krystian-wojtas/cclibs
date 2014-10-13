/*!
 * @file  sim.h
 * @brief Converter Control Regulation library voltage source and load simulation functions
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
#include <libreg/load.h>

// Constants

#define REG_N_VS_SIM_COEFFS                     4               //!< Number of Voltage Source simulation coefficients
#define REG_VS_SIM_UNDERSAMPLED_THRESHOLD       0.25            //!< Threshold for calculated VS delay in iteration periods

// Simulation structures

/*!
 * Load simulation parameters
 */
struct reg_sim_load_pars
{
    float                       tc_error;                       //!< Simulated load time constant error
    float                       period_tc_ratio;                //!< Simulation period / load time constant
    uint32_t                    load_undersampled_flag;         //!< Simulated load is under-sampled flag
    struct reg_load_pars        load_pars;                      //!< Simulated load parameters
};

/*!
 * Load simulation variables
 *
 * V_REF_DELAY is the delay between the start of an iteration in which the voltage
 * reference is calculated and the time that it enters the simulation of the voltage
 * source. This models the delay that might be due to a DAC settling, or a digital
 * link between a current controller and the voltage source electronics.
 */
struct reg_sim_load_vars
{
    float                       circuit_voltage;                //!< Circuit voltage (without V_REF_DELAY)
    float                       circuit_current;                //!< Circuit current (without V_REF_DELAY)
    float                       magnet_current;                 //!< Magnet current  (without V_REF_DELAY)
    float                       magnet_field;                   //!< Magnet field    (without V_REF_DELAY)
    float volatile              integrator;                     //!< Integrator for simulated current
    float                       compensation;                   //!< Compensation for Kahan Summation
};

/*!
 * Voltage source simulation parameters
 */
struct reg_sim_vs_pars
{
    float                       num  [REG_N_VS_SIM_COEFFS];     //!< Numerator coefficients b0, b1, b2, etc. See also #REG_N_VS_SIM_COEFFS
    float                       den  [REG_N_VS_SIM_COEFFS];     //!< Denominator coefficients a0, a2, a2, etc. See also #REG_N_VS_SIM_COEFFS
    float                       v_ref_delay_iters;              //!< Delay before the voltage reference is applied to the voltage source
    float                       vs_delay_iters;                 //!< Voltage source delay for steady ramp in iterations
    float                       gain;                           //!< \f[gain = \frac{\sum den}{\sum num}\f]
    uint32_t                    vs_undersampled_flag;           //!< Simulated voltage source is under-sampled flag
};

/*!
 * Voltage source simulation variables
 */
struct reg_sim_vs_vars
{
    float                       v_ref    [REG_N_VS_SIM_COEFFS]; //!< Voltage reference history. See also #REG_N_VS_SIM_COEFFS
    float                       v_circuit[REG_N_VS_SIM_COEFFS]; //!< Simulated circuit voltage history. See also #REG_N_VS_SIM_COEFFS
};

#ifdef __cplusplus
extern "C" {
#endif

// Simulation functions

/*!
 * Initialise the load simulation parameters structure.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[out]    sim_load_pars        Load simulation parameters object to update
 * @param[in]     load_pars            Load parameters
 * @param[in]     sim_load_tc_error    Simulation load time constant error. If the Tc
 *                                     error is zero, the simulation will simply use load_pars.
 *                                     Otherwise it will initialise the simulated load using
 *                                     distorted load_pars so that the simulated load time
 *                                     constant will mismatch the load_pars time constants by
 *                                     the required factor (i.e. 0.1 = 10% error in Tc).
 */
void regSimLoadInit(struct reg_sim_load_pars *sim_load_pars, struct reg_load_pars *load_pars, float sim_load_tc_error, float sim_period);

/*!
 * Initialises the load simulation with the field b_init.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in]     pars                 Load simulation parameters
 * @param[in,out] vars                 Load simulation values object to update
 * @param[in]     b_init               Initial field value
 */
void regSimLoadSetField(struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float b_init);

/*!
 * Initialise the load simulation with the current
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in]     pars                 Load simulation parameters
 * @param[in,out] vars                 Load simulation values object to update
 * @param[in]     i_init               Initial current value
 */
void regSimLoadSetCurrent(struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float i_init);

/*!
 * Initialise the load simulation with the load voltage
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in]     pars                 Load simulation parameters
 * @param[in,out] vars                 Load simulation values object to update
 * @param[in]     v_init               Initial load voltage value
 */
void regSimLoadSetVoltage(struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float v_init);

/*!
 * Initialise voltage source model. This function sets or clears
 * reg_sim_vs_pars::vs_undersampled_flag.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in,out] pars                 Load simulation parameters object to update
 * @param[in]     iter_period          Simulation iteration period
 * @param[in]     v_ref_delay_iters    Delay before the voltage reference is applied to the voltage source
 * @param[in]     bandwidth            VS 2nd order model bandwidth (-3 dB). Set to zero to use num,den.
 * @param[in]     z                    2nd order model damping
 * @param[in]     tau_zero             2nd order optional zero time constant. Set to zero if not used.
 * @param[in]     num                  VS model numerator coefficients (used if bandwidth is zero)
 * @param[in]     den                  VS model denominator coefficients (used if bandwidth is zero)
 */
void regSimVsInit(struct reg_sim_vs_pars *pars, double iter_period, float v_ref_delay_iters,
                  float bandwidth, float z, float tau_zero,
                  float num[REG_N_VS_SIM_COEFFS], float den[REG_N_VS_SIM_COEFFS]);

/*!
 * Initialise the voltage source simulation history to be in steady-state with the given
 * v_circuit value.
 *
 * <strong>Note:</strong> the gain must be calculated (using regSimVsInit()) before
 * calling this function.
 *
 * This is a non-Real-Time function: do not call from the real-time thread or interrupt
 *
 * @param[in]     pars                 Load simulation parameters
 * @param[out]    vars                 Load simulation values object to update
 * @param[in]     v_circuit            Load voltage
 * @returns Steady-state voltage reference
 */
float regSimVsInitHistory(struct reg_sim_vs_pars *pars, struct reg_sim_vs_vars *vars, float v_circuit);

/*!
 * Simulate the voltage source in response to the specified voltage reference.
 *
 * This is a Real-Time function (thread safe).
 *
 * @param[in]     pars                 Load simulation parameters
 * @param[in,out] vars                 Load simulation values object to update
 * @param[in]     v_ref                Voltage reference
 * @returns Load voltage
 */
float regSimVsRT(struct reg_sim_vs_pars *pars, struct reg_sim_vs_vars *vars, float v_ref);

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
 * @param[in]     pars                 Load simulation parameters
 * @param[in,out] vars                 Load simulation values object to update
 * @param[in]     vs_undersampled_flag Voltage Source undersampled flag. If zero, use first-order interpolation
 *                                     of voltage. If non-zero, voltage source is undersampled: use initial
 *                                     voltage for complete sample.
 * @param[in]     v_circuit            Load voltage, stored in reg_sim_load_vars::circuit_voltage for the next
 *                                     iteration.
 * @returns Circuit current
 */
float regSimLoadRT(struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, uint32_t vs_undersampled_flag, float v_circuit);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_SIM_H

// EOF

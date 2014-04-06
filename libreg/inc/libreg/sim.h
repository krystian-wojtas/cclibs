/*---------------------------------------------------------------------------------------------------------*\
  File:     libreg/sim.h                                                                Copyright CERN 2014

  License:  This file is part of libreg.

            libreg is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  Converter Control Regulation library simulation functions header file

  Contact:  cclibs-devs@cern.ch

\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBREG_SIM_H
#define LIBREG_SIM_H

#include <stdint.h>
#include <libreg/load.h>

// Constants

#define REG_N_VS_SIM_COEFFS     4                               // Number of Voltage Source simulation coefficients

// Simulation structures

struct reg_sim_load_pars                                        // Load simulation parameters
{
    float                       tc_error;                       // Simulated load time constant error
    float                       period_tc_ratio;                // Simulation period / load time constant
    uint32_t                    load_undersampled_flag;         // Simulated load is undersampled flag
    uint32_t                    vs_undersampled_flag;           // Simulated voltage source is undersampled flag

    struct reg_load_pars load_pars;                             // Simulated load parameters
};

struct reg_sim_load_vars                                        // Load simulation variables
{
    float                       voltage;                        // Load voltage from voltage source simulation
    float                       field;                          // Simulated magnet field
    float                       current;                        // Simulated circuit current
    float                       mag_current;                    // Simulated magnet current
    float volatile              integrator;                     // Integrator for simulated current
    float                       compensation;                   // Compensation for Kahan Summation
};

struct reg_sim_vs_pars                                          // Voltage source simulation parameters
{                                                               // Note: the order of fields is significant in fgtest
    float                       num  [REG_N_VS_SIM_COEFFS];     // Numerator coefficients b0, b1, b2, ...
    float                       den  [REG_N_VS_SIM_COEFFS];     // Denominator coefficients a0, a2, a2, ...
    float                       v_ref_delay_iters;              // Delay before V_REF is applied to the voltage source
    float                       step_rsp_time_iters;            // Time for step response to cross 50%
    float                       gain;                           // Gain = Sum(den)/Sum(num)
};

struct reg_sim_vs_vars                                          // Voltage source simulation variables
{
    float                       v_ref [REG_N_VS_SIM_COEFFS];    // Voltage reference history
    float                       v_load[REG_N_VS_SIM_COEFFS];    // Simulated load voltage history
};

#ifdef __cplusplus
extern "C" {
#endif

// Simulation functions

void     regSimLoadTcError      (struct reg_sim_load_pars *sim_load_pars, struct reg_load_pars *load_pars,
                                 float sim_load_tc_error);
void     regSimLoadInit         (struct reg_sim_load_pars *sim_load_pars, float sim_period);
void     regSimLoadSetField     (struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float b_init);
void     regSimLoadSetCurrent   (struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float i_init);
void     regSimLoadSetVoltage   (struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float v_init);
float    regSimLoad             (struct reg_sim_load_pars *pars, struct reg_sim_load_vars *vars, float voltage);
void     regSimVsInit           (struct reg_sim_vs_pars *pars, float sim_period, float bandwidth,
                                 float z, float tau_zero);
uint32_t regSimVsInitGain       (struct reg_sim_vs_pars   *pars, struct reg_sim_vs_vars   *vars, float v_ref_delay_iters);
float    regSimVsInitHistory    (struct reg_sim_vs_pars   *pars, struct reg_sim_vs_vars   *vars, float v_ref);
float    regSimVs               (struct reg_sim_vs_pars   *pars, struct reg_sim_vs_vars   *vars, float v_ref);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_SIM_H

// EOF

/*---------------------------------------------------------------------------------------------------------*\
  File:     libreg.h                                                                    Copyright CERN 2014

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

  Purpose:  Converter Control Regulation library header file

  Contact:  cclibs-devs@cern.ch

  Notes:    The regulation library provides support for:

                1. Field, Current and voltage limits
                2. RST based regulation (Landau notation)
                3. Regulation error calculation
                4. Voltage source simulation
                5. Magnet load definition and simulation

\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBREG_H
#define LIBREG_H

// Include header files

#include <stdint.h>
#include <libreg/delay.h>
#include <libreg/err.h>
#include <libreg/lim.h>
#include <libreg/load.h>
#include <libreg/meas.h>
#include <libreg/rst.h>
#include <libreg/sim.h>

// Regulation error rate control enum

enum reg_err_rate
{
    REG_ERR_RATE_REGULATION,                            ///< Calculate regulation error at regulation rate
    REG_ERR_RATE_MEASUREMENT                            ///< Calculate regulation error at measurement rate
};

// Global power converter regulation structures

struct reg_sim_meas                                     ///< Measurement simulation structure
{
    struct reg_delay            meas_delay;             ///< Measurement delay parameters
    struct reg_noise_and_tone   noise_and_tone;         ///< Simulated noise and tone parameters
    float                       signal;                 ///< Simulated measured signal with noise and tone
};

struct reg_signal
{
    struct reg_meas_filter      meas;                   ///< Unfiltered and filtered measurement (real or sim)
    struct reg_meas_rate        rate;                   ///< Estimation of the rate of the field measurement
    struct reg_lim_meas         lim_meas;               ///< Measurement limits
    struct reg_lim_ref          lim_ref;                ///< Reference limits
    struct reg_rst_pars         rst_pars;               ///< Regulation RST parameters
    enum   reg_err_rate         err_rate;               ///< Rate control for regulation error calculation
    struct reg_err              err;                    ///< Regulation error
    struct reg_sim_meas         sim;                    ///< Simulated measurement with noise and tone
};

struct reg_converter                                    ///< Global converter regulation structure
{
    double                      iter_period;            ///< Iteration period

    // Reference and regulation variables

    enum reg_mode               mode;                   ///< Regulation mode: Field, current or voltage
    struct reg_signal           *r;                     ///< Pointer to active regulation structure (reg.i or reg.b)

    uint32_t                    iteration_counter;      ///< Iteration counter (within each regulation period)
    uint32_t                    period_iters;           ///< Regulation period (in iterations)
    double                      period;                 ///< Regulation period
    double                      time;                   ///< Time of last regulation iteration

    float                       meas;                   ///< Field or current regulated measurement
    float                       ref_advance;            ///< Time to advance reference

    float                       ref;                    ///< Field or current reference
    float                       ref_limited;            ///< Field or current reference after limits
    float                       ref_rst;                ///< Field or current reference after back-calculation
    float                       ref_delayed;            ///< Field or current reference delayed by track_delay

    float                       v_ref;                  ///< Voltage reference before saturation or limits
    float                       v_ref_sat;              ///< Voltage reference after saturation compensation
    float                       v_ref_limited;          ///< Voltage reference after saturation and limits

    struct                                              ///< Reference (field, current or voltage) limit flags
    {
        uint32_t                ref_clip;               ///< Reference is being clipped
        uint32_t                ref_rate;               ///< Reference rate of change is being clipped
    } flags;

    // Field and current regulation structures

    struct reg_signal           b;                      ///< Field regulation parameters and variables
    struct reg_signal           i;                      ///< Current regulation parameters and variables

    // Regulation variables structures

    struct reg_rst_vars         rst_vars;               ///< Field or current regulation RST variables

    // Voltage reference, measurement and regulation error

    struct reg_lim_ref          lim_v_ref;              ///< Voltage reference limits
    float                       v_meas;                 ///< Unfiltered voltage measurement (real or sim)
    struct reg_err              v_err;                  ///< Voltage regulation error
    struct reg_sim_meas         v_sim;                  ///< Simulated voltage measurement with noise and tone

    // Load parameters and variables structures

    struct reg_load_pars        load_pars;              ///< Circuit load model for regulation

    struct reg_sim_vs_pars      sim_vs_pars;            ///< Voltage source simulation parameters
    struct reg_sim_load_pars    sim_load_pars;          ///< Circuit load model for simulation

    struct reg_sim_vs_vars      sim_vs_vars;            ///< Voltage source simulation variables
    struct reg_sim_load_vars    sim_load_vars;          ///< Load simulation variables
};

#ifdef __cplusplus
extern "C" {
#endif

// Converter regulation functions

void     regSetSimLoad          (struct reg_converter *reg, enum reg_mode reg_mode, float sim_load_tc_error);
void     regSetMeas             (struct reg_converter *reg, float v_meas, float i_meas, float b_meas, uint32_t sim_meas_control);
float    regCalcPureDelay       (struct reg_converter *reg, struct reg_meas_filter *meas_filter, uint32_t reg_period_iters);
void     regSetMode             (struct reg_converter *reg, enum reg_mode reg_mode);
uint32_t regConverter           (struct reg_converter *reg, float *ref, float feedforward_v_ref, uint32_t feedforward_control,
                                 uint32_t max_abs_err_control);
void     regSimulate            (struct reg_converter *reg, float v_perturbation);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_H
// EOF


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
    REG_ERR_RATE_REGULATION,
    REG_ERR_RATE_MEASUREMENT
};

// Global power converter regulation structures

struct reg_sim_meas                                             // Measurement simulation structure
{
    struct reg_delay            delay;                          // Simulated measurement delay parameters
    struct reg_noise_and_tone   noise_and_tone;                 // Simulated noise and tone parameters
    float                       load;                           // Simulated value in the load
    float                       meas;                           // Simulated measurement with noise and tone
};

struct reg_converter                                            // Global converter regulation structure
{
    enum reg_mode               mode;                           // Regulation mode: Field, current or voltage
    double                      iter_period;                    // Iteration period

    // Field, current and voltage measurements

    struct reg_meas_filter      b_meas;                         // Unfiltered and filtered field measurement (real or sim)
    struct reg_meas_filter      i_meas;                         // Unfiltered and filtered current measurement (real or sim)
    float                       v_meas;                         // Unfiltered voltage measurement (real or sim)

    // Field and current measurement rate

    struct reg_meas_rate        b_rate;                         // Estimation of the rate of the field measurement
    struct reg_meas_rate        i_rate;                         // Estimation of the rate of the current measurement

    // Reference and regulation variables

    double                      period;                         // Regulation period
    uint32_t                    period_iters;                   // Regulation period (in iterations)
    uint32_t                    iteration_counter;              // Iteration counter
    double                      time;                           // Time of last regulation iteration
    float                       ref_advance;                    // Time to advance reference
    float                       meas;                           // Field or current regulated measurement

    float                       ref;                            // Field or current reference
    float                       ref_limited;                    // Field or current reference after limits
    float                       ref_rst;                        // Field or current reference after back-calculation
    float                       ref_delayed;                    // Field or current reference delayed by track_delay

    float                       v_ref;                          // Voltage reference before saturation or limits
    float                       v_ref_sat;                      // Voltage reference after saturation compensation
    float                       v_ref_limited;                  // Voltage reference after saturation and limits

    float                       err;                            // Regulation error (field or current)
    float                       max_abs_err;                    // Max absolute regulation error (field or current)

    // Measurement and reference limits and flags

    struct reg_lim_meas         lim_i_meas;                     // Current measurement limits
    struct reg_lim_meas         lim_b_meas;                     // Field measurement limits

    struct reg_lim_ref          lim_v_ref;                      // Voltage reference limits
    struct reg_lim_ref          lim_i_ref;                      // Current reference limits
    struct reg_lim_ref          lim_b_ref;                      // Field reference limits

    struct                                                      // Reference (field, current or voltage) limit flags
    {
        uint32_t                ref_clip;                       // Reference is being clipped
        uint32_t                ref_rate;                       // Reference rate of change is being clipped
    } flags;

    // Regulation variables structures

    struct reg_rst_vars         rst_vars;                       // Field or current regulation RST variables

    enum reg_err_rate           b_err_rate;                     // Rate control for field regulation error calculation
    enum reg_err_rate           i_err_rate;                     // Rate control for current regulation error calculation

    struct reg_err              b_err;                          // Field regulation error
    struct reg_err              i_err;                          // Current regulation error
    struct reg_err              v_err;                          // Voltage regulation error

    // Simulation variables structures

    struct reg_sim_vs_vars      sim_vs_vars;                    // Voltage source simulation variables
    struct reg_sim_load_vars    sim_load_vars;                  // Load simulation variables

    struct reg_sim_meas         b_sim;                          // Simulated field measurement
    struct reg_sim_meas         i_sim;                          // Simulated current measurement
    struct reg_sim_meas         v_sim;                          // Simulated voltage measurement
};

// The parameter structures that are time consuming to calculate are included in the following structure
// which can be double buffered if required to avoid race conditions when regulating/simulating in real-time

struct reg_converter_pars                                       // Global converter regulation parameters structure
{
    struct reg_rst_pars         b_rst_pars;                     // Field regulation RST parameters
    struct reg_rst_pars         i_rst_pars;                     // Current regulation RST parameters
    struct reg_sim_vs_pars      sim_vs_pars;                    // Voltage source simulation parameters
    struct reg_load_pars        load_pars;                      // Circuit load model for regulation
    struct reg_sim_load_pars    sim_load_pars;                  // Circuit load model for simulation
};

#ifdef __cplusplus
extern "C" {
#endif

// Converter regulation functions

void     regSetSimLoad          (struct reg_converter *reg, struct reg_converter_pars *reg_pars,
                                 enum reg_mode reg_mode, float sim_load_tc_error);
void     regSetMeas             (struct reg_converter *reg,  struct reg_converter_pars *reg_pars,
                                 float v_meas, float i_meas, float b_meas, uint32_t sim_meas_control);
float    regCalcPureDelay       (struct reg_converter *reg, struct reg_converter_pars *reg_pars,
                                 struct reg_meas_filter *meas_filter, uint32_t reg_period_iters);
void     regSetMode             (struct reg_converter *reg, struct reg_converter_pars *reg_pars, enum reg_mode reg_mode);
uint32_t regConverter           (struct reg_converter *reg, struct reg_converter_pars *reg_pars, float *ref,
                                 float feedforward_v_ref, uint32_t feedforward_control,
                                 uint32_t max_abs_err_control);
void     regSimulate            (struct reg_converter *reg, struct reg_converter_pars *reg_pars,
                                 float v_perturbation);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_H
// EOF


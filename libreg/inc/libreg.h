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
                6. Signal delay

\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBREG_H
#define LIBREG_H

// Include header files

#include <stdint.h>
#include <libreg/delay.h>
#include <libreg/err.h>
#include <libreg/lim.h>
#include <libreg/load.h>
#include <libreg/rst.h>
#include <libreg/sim.h>

// Constants

#define REG_N_IIR_COEFFS        11                              // Number of Voltage Source simulation coefficients...
                                                                // ...if this is changed, the code in reg.c must be modified
// Measurement structures

struct reg_meas_pars                                            // Measurement filter parameters
{
    float                       num  [REG_N_IIR_COEFFS];        // Numerator coefficients b0, b1, b2, ...
    float                       den  [REG_N_IIR_COEFFS];        // Denominator coefficients a0, a2, a2, ...
    float                       num0_correction;                // Sum(den) - Sum(num) to compensate single precision floats
    uint32_t                    order;                          // Filter order (number of coefficients - 1)
};

struct reg_meas_vars                                            // Measurement filter variables
{
    uint32_t                    iir_latest_index;               // Index in IIR history of latest sample
    float                       unfiltered;                     // Unfiltered measurement at iteration period
    float                       iir_in [REG_N_IIR_COEFFS];      // IIR filtered measurement at iteration period
    float                       iir_out[REG_N_IIR_COEFFS];      // IIR filtered measurement at iteration period
    float                       filtered;                       // IIR filtered measurement at iteration period
    float                       accumulator;                    // Accumulator used to produce decimated value
    float                       regulated;                      // Measurement used for closed-loop regulation (raw or decimated)
};

// Global power converter regulation structure

struct reg_converter                                            // Global converter regulation structure
{
    enum reg_mode               mode;                           // Field, current or voltage regulation

    float                       iter_period;                    // Iteration period
    float                       cl_period;                      // Closed loop regulation period
    uint32_t                    cl_period_iters;                // Closed loop regulation period (in iterations)
    uint32_t                    iteration_counter;              // Iteration counter

    // Measured values - real or simulated

    struct reg_meas_vars        v_meas;                         // Voltage measurement
    struct reg_meas_vars        i_meas;                         // Current measurement
    struct reg_meas_vars        b_meas;                         // Field measurement

    // Reference and regulation variables

    float                       ref;                            // Field or current reference
    float                       ref_limited;                    // Field or current reference after limits
    float                       ref_rst;                        // Field or current reference after back-calculation
    float                       ref_prev;
    float                       ref_rate;
    float                       ref_interpolated;
    float                       v_ref;                          // Voltage reference before saturation or limits
    float                       v_ref_sat;                      // Voltage reference after saturation compensation
    float                       v_ref_limited;                  // Voltage reference after saturation and limits
    float                       f_ref;                          // Firing reference
    float                       f_ref_limited;                  // Firing reference after limits
    float                       err;                            // Regulation error (field or current)
    float                       max_abs_err;                    // Max absolute regulation error (field or current)

    // Measurement and reference limits

    struct reg_lim_meas         lim_i_meas;                     // Current measurement limits
    struct reg_lim_meas         lim_b_meas;                     // Field measurement limits

    struct reg_lim_ref          lim_v_ref;                      // Voltage reference limits
    struct reg_lim_ref          lim_i_ref;                      // Current reference limits
    struct reg_lim_ref          lim_b_ref;                      // Field reference limits

    struct                                                      // Reference (field, current or voltage) limit flags
    {
        uint32_t                ref_clip;                       // Ref is being clipped
        uint32_t                ref_rate;                       // Ref rate is being clipped
    } flags;

    // Regulation variables structures

    struct reg_rst_vars         rst_vars;                       // Field or current regulation RST variables
    struct reg_rst_vars         v_rst_vars;                     // Voltage regulation RST variables

    struct reg_err              v_err;                          // Voltage regulation error
    struct reg_err              i_err;                          // Current regulation error
    struct reg_err              b_err;                          // Field regulation error

    // Simulation variables structures

    struct reg_sim_vs_vars      sim_vs_vars;                    // Voltage source simulation variables
    struct reg_sim_load_vars    sim_load_vars;                  // Load simulation variables
    float                       sim_v_load;                     // Simulated load voltage

    struct reg_delay            v_meas_delay;                   // Voltage measurement delay parameters
    struct reg_delay            i_meas_delay;                   // Current measurement delay parameters
    struct reg_delay            b_meas_delay;                   // Field measurement delay parameters

    float                       v_meas_noise;                   // Noise amplitude for simulated voltage measurement
    float                       b_meas_noise;                   // Noise amplitude for simulated field measurement
    float                       i_meas_noise;                   // Noise amplitude for simulated current measurement
};

// The parameter structures that are time consuming to calculate are included in the following structure
// which can be double buffered if required to avoid race conditions when regulating/simulating in real-time

struct reg_converter_pars                                       // Global converter regulation parameters structure
{
    struct reg_rst_pars         v_rst_pars;                     // Voltage regulation RST parameters
    struct reg_rst_pars         i_rst_pars;                     // Current regulation RST parameters
    struct reg_rst_pars         b_rst_pars;                     // Field regulation RST parameters
    struct reg_sim_vs_pars      sim_vs_pars;                    // Voltage source simulation parameters
    struct reg_load_pars        load_pars;                      // Circuit load model for regulation
    struct reg_sim_load_pars    sim_load_pars;                  // Circuit load model for simulation
    struct reg_meas_pars        v_meas;
    struct reg_meas_pars        i_meas;
    struct reg_meas_pars        b_meas;
};

#ifdef __cplusplus
extern "C" {
#endif

// Converter regulation functions

void     regMeasFilterInit       (struct reg_meas_pars *pars, struct reg_meas_vars *vars, float  num[REG_N_IIR_COEFFS], float den[REG_N_IIR_COEFFS]);
void     regMeasFilterInitHistory(struct reg_meas_vars *vars, float init_meas);
float    regCalcErrDelay         (struct reg_rst_pars *rst_pars);
void     regSetSimLoad           (struct reg_converter *reg, struct reg_converter_pars *reg_pars,
                                  enum reg_mode reg_mode, float sim_load_tc_error);
void     regSetLoad              (float ohms_ser, float ohms_par, float ohms_mag, float henrys,
                                  float gauss_per_amp, float sim_load_tc_error);
void     regSetMeasNoise         (struct reg_converter *reg, float v_meas_noise, float b_meas_noise, float i_meas_noise);
void     regSetMeas              (struct reg_converter *reg,  struct reg_converter_pars *reg_pars,
                                  float v_meas, float i_meas, float b_meas, uint32_t sim_meas_control);
void     regSetVoltageMode       (struct reg_converter *reg, struct reg_converter_pars *reg_pars);
void     regSetMode              (struct reg_converter *reg, struct reg_converter_pars *reg_pars,
                                  enum reg_mode mode, float meas, float rate);
uint32_t regConverter            (struct reg_converter *reg, struct reg_converter_pars *reg_pars, float ref,
                                  float feedforward_v_ref, uint32_t feedforward_control,
                                  uint32_t max_abs_err_control);
void     regSimulate             (struct reg_converter *reg, struct reg_converter_pars *reg_pars,
                                  float v_perturbation);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_H

// EOF

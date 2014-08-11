/*---------------------------------------------------------------------------------------------------------*\
  File:     libreg/conv.h                                                               Copyright CERN 2014

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
\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBREG_CONV_H
#define LIBREG_CONV_H

// Include header files

#include <stdint.h>

// Regulation parameters source (operational or test) enum

enum reg_actuation
{
    REG_VOLTAGE_REF,                                    ///< Actuation is a voltage reference
    REG_CURRENT_REF                                     ///< Actuation is a current reference
};

// Regulation parameters source (operational or test) enum

enum reg_rst_source
{
    REG_OPERATIONAL_RST_PARS,                           ///< Use operational RST parameters
    REG_TEST_RST_PARS                                   ///< Use test RST parameters
};

// Regulation error rate control enum

enum reg_err_rate
{
    REG_ERR_RATE_REGULATION,                            ///< Calculate regulation error at regulation rate
    REG_ERR_RATE_MEASUREMENT                            ///< Calculate regulation error at measurement rate
};

// Global power converter regulation structures

struct reg_conv_sim_meas                                ///< Measurement simulation structure
{
    struct reg_delay            meas_delay;             ///< Measurement delay parameters
    struct reg_noise_and_tone   noise_and_tone;         ///< Simulated noise and tone parameters
    float                       signal;                 ///< Simulated measured signal with noise and tone
};

struct reg_conv_rst_pars
{
    uint32_t                    use_next_pars;          ///< Signal to use next RST pars in the RT thread
    struct reg_rst_pars        *active;                 ///< Pointer to active parameters in pars[]
    struct reg_rst_pars        *next;                   ///< Pointer to next parameters in pars[]
    struct reg_rst_pars        *debug;                  ///< Pointer to most recently initialized parameters in pars[]
    struct reg_rst_pars         pars[2];                ///< Structures for active and next RST parameter
};

struct reg_conv_signal                                  ///< Converter signal (field or current) regulation structure
{
    struct reg_meas_signal     *input_p;                ///< Pointer to input measurement signal structure
    struct reg_meas_signal      input;                  ///< Input measurement and measurement status
    uint32_t                    invalid_input_counter;  ///< Counter for invalid input measurements
    struct reg_meas_filter      meas;                   ///< Unfiltered and filtered measurement (real or sim)
    struct reg_meas_rate        rate;                   ///< Estimation of the rate of the field measurement
    struct reg_lim_meas         lim_meas;               ///< Measurement limits
    struct reg_lim_ref          lim_ref;                ///< Reference limits
    struct reg_rst_pars        *rst_pars;               ///< Active RST parameters (Active Operational or Test)
    struct reg_conv_rst_pars    op_rst_pars;            ///< Operational regulation RST parameters
    struct reg_conv_rst_pars    test_rst_pars;          ///< Test regulation RST parameters
    enum   reg_err_rate         err_rate;               ///< Rate control for regulation error calculation
    struct reg_err              err;                    ///< Regulation error
    struct reg_conv_sim_meas    sim;                    ///< Simulated measurement with noise and tone
};

struct reg_conv_voltage                                 ///< Converter voltage structure
{
    struct reg_meas_signal     *input_p;                ///< Pointer to input measurement signal structure
    struct reg_meas_signal      input;                  ///< Input measurement and measurement status
    uint32_t                    invalid_input_counter;  ///< Counter for invalid input measurements
    float                       meas;                   ///< Unfiltered voltage measurement (real or sim)
    struct reg_lim_ref          lim_ref;                ///< Voltage reference limits
    struct reg_rst_pars         rst_pars;               ///< Regulation RST parameters
    enum   reg_err_rate         err_rate;               ///< Rate control for regulation error calculation
    struct reg_err              err;                    ///< Voltage regulation error
    struct reg_conv_sim_meas    sim;                    ///< Simulated voltage measurement with noise and tone
    float                       ref;                    ///< Voltage reference before saturation or limits
    float                       ref_sat;                ///< Voltage reference after saturation compensation
    float                       ref_limited;            ///< Voltage reference after saturation and limits
};

struct reg_conv                                         ///< Global converter regulation structure
{
    double                      iter_period;            ///< Iteration (measurement) period

    // Regulation reference and measurement variables and parameters

    enum   reg_actuation		actuation;				///< Converter actuation: Voltage reference or current reference
    enum   reg_mode             reg_mode;               ///< Regulation mode: Field, Current or Voltage
    enum   reg_rst_source       reg_rst_source;         ///< RST parameter source (Operational or Test)
    struct reg_conv_signal     *reg_signal;             ///< Pointer to currently regulated signal structure (reg.i or reg.b)

    uint32_t                    iteration_counter;      ///< Iteration counter (within each regulation period)
    double                      reg_period;             ///< Regulation period
    float                       ref_advance;            ///< Time to advance reference function

    float                       meas;                   ///< Field or current regulated measurement
    float                       ref;                    ///< Field or current reference
    float                       ref_limited;            ///< Field or current reference after limits
    float                       ref_rst;                ///< Field or current reference after back-calculation
    float                       ref_delayed;            ///< Field or current reference delayed by track_delay

    struct                                              ///< Reference (field, current or voltage) limit flags
    {
        uint32_t                ref_clip;               ///< Reference is being clipped
        uint32_t                ref_rate;               ///< Reference rate of change is being clipped
    } flags;

    struct reg_rst_vars         rst_vars;               ///< Field or current regulation RST variables

    // Field, current and voltage regulation structures

    struct reg_conv_signal      b;                      ///< Field regulation parameters and variables
    struct reg_conv_signal      i;                      ///< Current regulation parameters and variables
    struct reg_conv_voltage     v;                      ///< Voltage regulation parameters and variables
                                                        ///< (Voltage is regulated by voltage source)
    // Load parameters and variables structures

    struct reg_load_pars        load_pars;              ///< Circuit load model for regulation

    struct reg_sim_vs_pars      sim_vs_pars;            ///< Voltage source simulation parameters
    struct reg_sim_load_pars    sim_load_pars;          ///< Circuit load model for simulation

    struct reg_sim_vs_vars      sim_vs_vars;            ///< Voltage source simulation variables
    struct reg_sim_load_vars    sim_load_vars;          ///< Load simulation variables
};

// Converter control macro "functions"

#define regConvInitRefGenFunc(reg_p, reg_ref_gen_function) (reg_p)->reg_ref_gen_function=reg_ref_gen_function

// Converter control functions

#ifdef __cplusplus
extern "C" {
#endif

void     regConvInit              (struct reg_conv *conv, double iter_period, enum reg_actuation actuation);
uint32_t regConvRstInit           (struct reg_conv *conv, struct reg_conv_signal *reg_signal,
                                   enum reg_mode reg_mode, enum reg_rst_source reg_rst_source, uint32_t reg_period_iters,
                                   float auxpole1_hz, float auxpoles2_hz, float auxpoles2_z, float auxpole4_hz, float auxpole5_hz,
                                   float pure_delay_periods, float track_delay_periods,
                                   double manual_r[REG_N_RST_COEFFS], double manual_s[REG_N_RST_COEFFS], double manual_t[REG_N_RST_COEFFS]);
void     regConvInitSimLoad       (struct reg_conv *conv, enum reg_mode reg_mode, float sim_load_tc_error);
void     regConvInitMeas          (struct reg_conv *conv, struct reg_meas_signal *v_meas_p, struct reg_meas_signal *i_meas_p, struct reg_meas_signal *b_meas_p);

void     regConvSetModeRT         (struct reg_conv *conv, enum reg_mode reg_mode, enum reg_rst_source reg_rst_source, uint32_t iteration_counter);
uint32_t regConvSetMeasRT         (struct reg_conv *conv, uint32_t sim_meas_control);
uint32_t regConvRegulateRT        (struct reg_conv *conv, float *ref, uint32_t enable_max_abs_err);
void     regConvSimulateRT        (struct reg_conv *conv, float v_perturbation);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_CONV_H
// EOF


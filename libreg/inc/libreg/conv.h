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

struct reg_conv_signal
{
    struct reg_meas_filter      meas;                   ///< Unfiltered and filtered measurement (real or sim)
    struct reg_meas_rate        rate;                   ///< Estimation of the rate of the field measurement
    struct reg_lim_meas         lim_meas;               ///< Measurement limits
    struct reg_lim_ref          lim_ref;                ///< Reference limits
    struct reg_rst_pars         rst_pars;               ///< Regulation RST parameters
    enum   reg_err_rate         err_rate;               ///< Rate control for regulation error calculation
    struct reg_err              err;                    ///< Regulation error
    struct reg_conv_sim_meas    sim;                    ///< Simulated measurement with noise and tone
};

struct reg_conv_voltage
{
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

    enum   reg_mode             mode;                   ///< Regulation mode: Field, Current or Voltage
    struct reg_conv_signal     *r;                      ///< Pointer to active regulation structure (reg.i or reg.b)

    uint32_t                    iteration_counter;      ///< Iteration counter (within each regulation period)
    double                      period;                 ///< Regulation period
    double                      time;                   ///< Time of last regulation iteration
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

#ifdef __cplusplus
extern "C" {
#endif

// Converter regulation functions

float    regConvPureDelay       (struct reg_conv *reg, struct reg_meas_filter *meas_filter, uint32_t reg_period_iters);
void     regConvInitSimLoad     (struct reg_conv *reg, enum reg_mode reg_mode, float sim_load_tc_error);

void     regConvSetMeasRT       (struct reg_conv *reg, float v_meas, float i_meas, float b_meas, uint32_t sim_meas_control);
void     regConvSetModeRT       (struct reg_conv *reg, enum reg_mode reg_mode, uint32_t iteration_counter);
uint32_t regConverterRT         (struct reg_conv *reg, float *ref, float feedforward_v_ref, uint32_t feedforward_control, uint32_t max_abs_err_control);
void     regConvSimulateRT      (struct reg_conv *reg, float v_perturbation);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_CONV_H
// EOF


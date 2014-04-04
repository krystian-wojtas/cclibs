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

// Measurement structures

struct reg_meas_filter                                          // Measurement filter parameters and variables
{
    uint32_t                    stop_iters;                     // Stop filter for specified number of iters
    uint32_t                    extrapolate_flag;               // Use extrapolated measurement for regulation flag
    uint32_t                    extrapolation_len_iters;        // Extrapolation length (normally regulation period)
    uint32_t                    extrapolation_index;            // Index to oldest sample in extrapolation buffer
    uint32_t                    fir_length[2];                  // Filter length for two cascaded stages
    uint32_t                    fir_index[2];                   // Index to oldest sample in FIR buffers
    int32_t                     accumulator[2];                 // Filter accumulator for two cascaded stages
    int32_t                    *fir_buf[2];                     // Pointer to circular buffers for two cascaded FIR stage
    float                      *extrapolation_buf;              // Pointer to circular buffer for extrapolation
    float                       meas_delay_iters;               // Measurement delay in iterations
    float                       fir_delay_iters;                // FIR filter delay in iterations
    float                       unfiltered;                     // Unfiltered measurement at iteration rate
    float                       filtered;                       // FIR filtered measurement at iteration rate
    float                       extrapolated;                   // Extrapolated measurement at iteration rate
    float                       max_value;                      // Maximum value that can be filtered
    float                       float_to_integer;               // Factor to convert unfiltered measurement to integer
    float                       integer_to_float;               // Factor to converter integer to filtered measurement
    float                       extrapolation_factor;           // Extrapolation factor
};

struct reg_noise_and_tone                                        // Noise and tone generator structure
{
    uint32_t                    iter_counter;                   // Iteration counter for simulated tone
    uint32_t                    tone_half_period_iters;         // Tone half period in iterations
    uint32_t                    tone_toggle;                    // Tone toggle (0,1,0,1,...)
    float                       tone_amp;                       // Tone amplitude
    float                       noise_pp;                       // Simulated measurement peak-peak noise level
};

struct reg_sim_meas                                             // Measurement simulation structure
{
    struct reg_delay            delay;                          // Simulated measurement delay parameters
    struct reg_noise_and_tone   noise_and_tone;                 // Simulated noise and tone parameters
    float                       load;                           // Simulated value in the load
    float                       meas;                           // Simulated measurement with noise and tone
};

// Global power converter regulation structure

struct reg_converter                                            // Global converter regulation structure
{
    enum reg_mode               mode;                           // Field, current or voltage regulation

    float                       iter_period;                    // Iteration period
    float                       period;                         // Regulation period
    uint32_t                    period_iters;                   // Regulation period (in iterations)
    uint32_t                    iteration_counter;              // Iteration counter

    float                       v_meas;                         // Unfiltered voltage measurement (real or sim)
    struct reg_meas_filter      i_meas;                         // Unfiltered and filtered current measurement (real or sim)
    struct reg_meas_filter      b_meas;                         // Unfiltered and filtered field measurement (real or sim)

    // Reference and regulation variables

    float                       ref;                            // Field or current reference
    float                       ref_limited;                    // Field or current reference after limits
    float                       ref_rst;                        // Field or current reference after back-calculation
    float                       ref_prev;                       // Previous field or current reference
    float                       ref_rate;                       // Rate of change of field or current reference
    float                       ref_interpolated;               // Reference interpolated at iteration rate
    float                       meas;                           // Field or current regulated measurement
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

    struct reg_err              v_err;                          // Voltage regulation error
    struct reg_err              i_err;                          // Current regulation error
    struct reg_err              b_err;                          // Field regulation error

    // Simulation variables structures

    struct reg_sim_vs_vars      sim_vs_vars;                    // Voltage source simulation variables
    struct reg_sim_load_vars    sim_load_vars;                  // Load simulation variables
    float                       sim_v_load;                     // Simulated load voltage

    struct reg_sim_meas         v_sim;                          // Simulated voltage measurement
    struct reg_sim_meas         i_sim;                          // Simulated current measurement
    struct reg_sim_meas         b_sim;                          // Simulated field measurement
};

// The parameter structures that are time consuming to calculate are included in the following structure
// which can be double buffered if required to avoid race conditions when regulating/simulating in real-time

struct reg_converter_pars                                       // Global converter regulation parameters structure
{
    struct reg_rst_pars         i_rst_pars;                     // Current regulation RST parameters
    struct reg_rst_pars         b_rst_pars;                     // Field regulation RST parameters
    struct reg_sim_vs_pars      sim_vs_pars;                    // Voltage source simulation parameters
    struct reg_load_pars        load_pars;                      // Circuit load model for regulation
    struct reg_sim_load_pars    sim_load_pars;                  // Circuit load model for simulation
};

#ifdef __cplusplus
extern "C" {
#endif

// Converter regulation functions

void     regMeasFilterInitBuffer (struct reg_meas_filter *filter, int32_t *buf);
void     regMeasFilterInitOrders (struct reg_meas_filter *filter, uint32_t fir_length[2]);
void     regSetMeasNoise         (struct reg_converter *reg, float v_sim_noise, float i_sim_noise, float b_sim_noise);
void     regMeasFilterInitMax    (struct reg_meas_filter *filter, float pos, float neg);
void     regMeasFilterInitHistory(struct reg_meas_filter *filter);
void     regSetSimLoad           (struct reg_converter *reg, struct reg_converter_pars *reg_pars,
                                  enum reg_mode reg_mode, float sim_load_tc_error);
void     regSetLoad              (float ohms_ser, float ohms_par, float ohms_mag, float henrys,
                                  float gauss_per_amp, float sim_load_tc_error);
void     regSetNoiseAndTone      (struct reg_noise_and_tone *noise_and_tone, float noise_pp,
                                  float tone_amp, uint32_t tone_half_period_iters);
void     regSetMeas              (struct reg_converter *reg,  struct reg_converter_pars *reg_pars,
                                  float v_meas, float i_meas, float b_meas, uint32_t sim_meas_control);
void     regSetVoltageMode       (struct reg_converter *reg, struct reg_converter_pars *reg_pars);
void     regSetMode              (struct reg_converter *reg, struct reg_converter_pars *reg_pars,
                                  enum reg_mode mode, float meas, float rate);
uint32_t regConverter            (struct reg_converter *reg, struct reg_converter_pars *reg_pars, float *ref,
                                  float feedforward_v_ref, uint32_t feedforward_control,
                                  uint32_t max_abs_err_control);
void     regSimulate             (struct reg_converter *reg, struct reg_converter_pars *reg_pars,
                                  float v_perturbation);
float    regNoiseAndTone         (struct reg_noise_and_tone *noise_and_tone);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_H
// EOF


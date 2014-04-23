/*---------------------------------------------------------------------------------------------------------*\
  File:     libreg/rst.h                                                                    Copyright CERN 2014

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

  Purpose:  Converter Control Regulation library RST functions header file

  Contact:  cclibs-devs@cern.ch

  Notes:    The RST algorithm is implemented based on Landau notation:

                ACTUATION x S = REFERENCE x T - MEASUREMENT x R

            The other common notation swaps the R and S polynomials.

            The library uses 32-bit floating point for most of the floating point variables.
            However, there are critical sections of the RST computation that require higher
            precision.  40-bit is sufficient - this is the level available on the TI TMS320C32 DSP.
            On newer processors, 64-bit double precision is needed.

\*---------------------------------------------------------------------------------------------------------*/

#ifndef LIBREG_RST_H
#define LIBREG_RST_H

#include <stdint.h>
#include <libreg/load.h>

// Constants

#define REG_N_RST_COEFFS        10                              // RST order + 1
#define REG_RST_HISTORY_MASK    15                              // History buffer index mask
#define REG_AVE_V_REF_LEN       4                               // Number of iterations over which to average V_REF
#define REG_TRACK_DELAY_FLTR_TC 100                             // Track delay measurement filter time constant (periods)

// Regulation RST algorithm structures

enum reg_status                                                 // Regulation status
{
    REG_OK,
    REG_WARNING,
    REG_FAULT
};

enum reg_mode                                                   // Converter regulation mode
{
    REG_NONE,                                                   // No regulation mode set
    REG_VOLTAGE,                                                // Open loop (voltage reference)
    REG_CURRENT,                                                // Closed loop on current
    REG_FIELD                                                   // Closed loop on field
};

struct reg_rst                                                  // RST polynomial arrays and track delay
{
    float                       r[REG_N_RST_COEFFS];            // R polynomial coefficients (meas)
    float                       s[REG_N_RST_COEFFS];            // S polynomial coefficients (act)
    float                       t[REG_N_RST_COEFFS];            // T polynomial coefficients (ref)
};

struct reg_rst_pars                                             // RST algorithm parameters
{
    enum reg_status             status;                         // Regulation parameters status
    enum reg_mode               reg_mode;                       // Regulation mode (REG_CURRENT | REG_FIELD)
    uint32_t                    period_iters;                   // Regulation period (in iterations)
    uint32_t                    alg_index;                      // Algorithm index (1-5) - based on pure delay
    uint32_t                    dead_beat;                      // 0 = not dead-beat, 1-3 = dead-beat(1-3)
    double                      period;                         // Regulation period
    float                       inv_period_iters;               // 1/period_iters
    float                       ref_advance;                    // Reference advance time
    float                       pure_delay_periods;             // Pure delay in regulation periods
    float                       track_delay_periods;            // Track delay in regulation periods
    float                       inv_s0;                         // Store 1/S[0]
    float                       t0_correction;                  // Correction to t[0] for rounding errors
    float                       inv_corrected_t0;               // Store 1/(T[0]+ t0_correction)
    struct reg_rst              rst;                            // RST polynomials
};

struct reg_rst_vars                                             // RST algorithm variables
{
    uint32_t                    history_index;                  // Index to latest entry in the history
    uint32_t                    delayed_ref_index;              // Iteration counter for delayed reference
    float                       meas_track_delay_periods;       // Measured track_delay in regulation periods
    float                       filtered_track_delay_periods;   // Filtered measured track_delay in regulation periods
    float                       ref [REG_RST_HISTORY_MASK+1];   // RST calculated reference history
    float                       meas[REG_RST_HISTORY_MASK+1];   // RST measurement history
    float                       act [REG_RST_HISTORY_MASK+1];   // RST actuation history
};

#ifdef __cplusplus
extern "C" {
#endif

// RST regulation functions

uint32_t regRstInit             (struct reg_rst_pars *pars, double iter_period, uint32_t period_iters,
                                 struct reg_load_pars *load, float clbw, float clbw2, float z, float clbw3, float clbw4,
                                 float pure_delay_periods, float track_delay_periods, enum reg_mode reg_mode,
                                 struct reg_rst *manual);
float    regRstCalcAct          (struct reg_rst_pars *pars, struct reg_rst_vars *vars, float ref, float meas);
float    regRstCalcRef          (struct reg_rst_pars *pars, struct reg_rst_vars *vars, float act, float meas);
void     regRstHistory          (struct reg_rst_vars *vars);
float    regRstPrevRef          (struct reg_rst_vars *vars);
float    regRstDeltaRef         (struct reg_rst_vars *vars);
float    regRstDelayedRef       (struct reg_rst_pars *pars, struct reg_rst_vars *vars, float track_delay);
float    regRstAverageVref      (struct reg_rst_vars *vars);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_RST_H

// EOF

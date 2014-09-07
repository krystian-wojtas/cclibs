/*!
 * @file  rst.h
 * @brief Converter Control Regulation library RST functions header file
 *
 * The RST algorithm is implemented based on Landau notation:
 * 
 * ACTUATION x S = REFERENCE x T - MEASUREMENT x R
 * 
 * The other common notation swaps the R and S polynomials.
 * 
 * The library uses 32-bit floating point for most of the floating point variables.
 * However, there are critical sections of the RST computation that require higher
 * precision. 40-bit is sufficient - this is the level available on the TI TMS320C32 DSP.
 * On newer processors, 64-bit double precision is needed.
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

#ifndef LIBREG_RST_H
#define LIBREG_RST_H

#include <stdint.h>
#include <libreg/load.h>

// Constants

#define REG_N_RST_COEFFS           10                           //!< RST order + 1 (must be <= REG_RST_HISTORY_MASK)
#define REG_RST_HISTORY_MASK       31                           //!< History buffer index mask (must be 2^N-1)
#define REG_MM_WARNING_THRESHOLD   0.4                          //!< REG_WARNING level for Modulus Margin

// Regulation RST algorithm structures

enum reg_status                                                 //!< Regulation status
{
    REG_OK,
    REG_WARNING,
    REG_FAULT
};

enum reg_mode                                                   //!< Converter regulation mode
{
    REG_VOLTAGE,                                                //!< Open loop (voltage reference)
    REG_CURRENT,                                                //!< Closed loop on current
    REG_FIELD,                                                  //!< Closed loop on field
    REG_NONE                                                    //!< No regulation mode set
};

struct reg_rst                                                  //!< RST polynomial arrays and track delay
{
    double                      r[REG_N_RST_COEFFS];            //!< R polynomial coefficients (meas)
    double                      s[REG_N_RST_COEFFS];            //!< S polynomial coefficients (act)
    double                      t[REG_N_RST_COEFFS];            //!< T polynomial coefficients (ref)
};

struct reg_rst_pars                                             //!< RST algorithm parameters
{
    enum reg_status             status;                         //!< Regulation parameters status
    enum reg_mode               reg_mode;                       //!< Regulation mode (REG_CURRENT | REG_FIELD)
    uint32_t                    reg_period_iters;               //!< Regulation period (in iterations)
    uint32_t                    alg_index;                      //!< Algorithm index (1-5) - based on pure delay
    uint32_t                    dead_beat;                      //!< 0 = not dead-beat, 1-3 = dead-beat(1-3)
    double                      reg_period;                     //!< Regulation period
    float                       inv_reg_period_iters;           //!< 1/reg_period_iters
    float                       ref_advance;                    //!< Reference advance time
    float                       pure_delay_periods;             //!< Pure delay in regulation periods
    float                       track_delay_periods;            //!< Track delay in regulation periods
    float                       ref_delay_periods;              //!< Ref delay in regulation periods
    double                      inv_s0;                         //!< Store 1/S[0]
    double                      t0_correction;                  //!< Correction to t[0] for rounding errors
    double                      inv_corrected_t0;               //!< Store 1/(T[0]+ t0_correction)
    struct reg_rst              rst;                            //!< RST polynomials
    double                      a   [REG_N_RST_COEFFS];         //!< Plant numerator
    double                      b   [REG_N_RST_COEFFS];         //!< Plant denominator
    double                      as  [REG_N_RST_COEFFS];         //!< A.S
    double                      asbr[REG_N_RST_COEFFS];         //!< A.S + B.R
    int32_t                     jurys_result;                   //!< Jury's test result index (0=OK)
    float                       min_auxpole_hz;
    float                       modulus_margin;                 //!< Modulus margin = min(abs_S_p_y)
    float                       modulus_margin_freq;            //!< Frequency for Modulus margin
};

struct reg_rst_vars                                             //!< RST algorithm variables
{
    uint32_t                    history_index;                  //!< Index to latest entry in the history
    float                       prev_ref_rate;                  //!< Ref rate from previous iteration
    float                       meas_track_delay_periods;       //!< Measured track_delay in regulation periods
    float                       ref [REG_RST_HISTORY_MASK+1];   //!< RST calculated reference history
    float                       meas[REG_RST_HISTORY_MASK+1];   //!< RST measurement history
    float                       act [REG_RST_HISTORY_MASK+1];   //!< RST actuation history
};

// RST macro "functions"

#define regRstIncHistoryIndexRT(rst_vars_p) (rst_vars_p)->history_index = ((rst_vars_p)->history_index + 1) & REG_RST_HISTORY_MASK
#define regRstPrevRefRT(rst_vars_p) (rst_vars_p)->ref[(rst_vars_p)->history_index]
#define regRstDeltaRefRT(rst_vars_p) (regRstPrevRefRT(rst_vars_p) - (rst_vars_p)->ref[((rst_vars_p)->history_index - 1) & REG_RST_HISTORY_MASK])

// RST regulation functions

#ifdef __cplusplus
extern "C" {
#endif

enum reg_status regRstInit        (struct reg_rst_pars *pars, double iter_period, uint32_t reg_period_iters,
                                   struct reg_load_pars *load, float auxpole1_hz, float auxpoles2_hz, float auxpoles2_z,
                                   float auxpole4_hz, float auxpole5_hz,
                                   float pure_delay_periods, float track_delay_periods, enum reg_mode reg_mode,
                                   struct reg_rst *manual);
float    regRstCalcActRT          (struct reg_rst_pars *pars, struct reg_rst_vars *vars, float ref, float meas);
float    regRstCalcRefRT          (struct reg_rst_pars *pars, struct reg_rst_vars *vars, float act, float meas);
void     regRstTrackDelayRT       (struct reg_rst_vars *vars, float period, float max_rate);
float    regRstDelayedRefRT       (struct reg_rst_pars *pars, struct reg_rst_vars *vars, uint32_t iteration_index);
float    regRstAverageVrefRT      (struct reg_rst_vars *vars);

#ifdef __cplusplus
}
#endif

#endif // LIBREG_RST_H

// EOF

/*!
 * @file  conv.c
 * @brief Converter Control Regulation library higher-level functions.
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

// Define LIBREG_CONV_SOURCE to define regConvParsInit() in pars.h

#define LIBREG_CONV_SOURCE

// Include header files

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "libreg.h"
#include "init_pars.h"

// Static function declarations

static void regConvSetModeNoneOrVoltageRT(struct reg_conv *conv, enum reg_mode reg_mode);
static void regConvPrepareSignalRT(struct reg_conv *conv, enum reg_mode reg_mode, uint32_t unix_time, uint32_t us_time);



// Non-Real-Time Functions - do not call these from the real-time thread or interrupt

void regConvInit(struct reg_conv *conv, uint32_t iter_period_us,
                 enum reg_enabled_disabled field_regulation, enum reg_enabled_disabled current_regulation)
{
    conv->iter_period_us = iter_period_us;
    conv->iter_period    = iter_period_us * 1.0E-6;

    conv->b.regulation   = field_regulation;
    conv->i.regulation   = current_regulation;

    conv->reg_mode       = REG_NONE;
    conv->reg_rst_source = REG_OPERATIONAL_RST_PARS;
    conv->reg_signal     = &conv->i;

    // Prepare regulation parameter next/active pointers

    conv->b.op_rst_pars.next     = &conv->b.op_rst_pars.pars[0];
    conv->b.op_rst_pars.active   = &conv->b.op_rst_pars.pars[1];
    conv->b.op_rst_pars.use_next_pars = 0;

    conv->b.test_rst_pars.next   = &conv->b.test_rst_pars.pars[0];
    conv->b.test_rst_pars.active = &conv->b.test_rst_pars.pars[1];
    conv->b.test_rst_pars.use_next_pars = 0;

    conv->i.op_rst_pars.next     = &conv->i.op_rst_pars.pars[0];
    conv->i.op_rst_pars.active   = &conv->i.op_rst_pars.pars[1];
    conv->i.op_rst_pars.use_next_pars = 0;

    conv->i.test_rst_pars.next   = &conv->i.test_rst_pars.pars[0];
    conv->i.test_rst_pars.active = &conv->i.test_rst_pars.pars[1];
    conv->i.test_rst_pars.use_next_pars = 0;

    conv->b.rst_pars = conv->b.op_rst_pars.active;
    conv->i.rst_pars = conv->i.op_rst_pars.active;

    regConvSetModeNoneOrVoltageRT(conv, REG_NONE);

    // Initialize libreg parameter structures in conv and set par_mask so that all init functions are executed

    regConvParsInit(conv);
}



static void regConvRstInit(struct reg_conv        *conv,
                           enum reg_mode           reg_mode,
                           enum reg_rst_source     reg_rst_source,
                           uint32_t                reg_period_iters,
                           float                   auxpole1_hz,
                           float                   auxpoles2_hz,
                           float                   auxpoles2_z,
                           float                   auxpole4_hz,
                           float                   auxpole5_hz,
                           float                   pure_delay_periods,
                           float                   track_delay_periods,
                           double                  manual_r[REG_N_RST_COEFFS],
                           double                  manual_s[REG_N_RST_COEFFS],
                           double                  manual_t[REG_N_RST_COEFFS])
{
    struct reg_conv_rst_pars   *conv_rst_pars;
    struct reg_conv_signal     *reg_signal;
    struct reg_load_pars       *load_pars;

    // Set pointer to the reg_conv_rst_pars structure for FIELD/CURRENT regulation with OPERATIONAL/TEST parameters

    reg_signal = (reg_mode == REG_FIELD ? &conv->b : &conv->i);

    if(reg_rst_source == REG_OPERATIONAL_RST_PARS)
    {
        conv_rst_pars = &reg_signal->op_rst_pars;
        load_pars     = &conv->load_pars;
    }
    else // REG_TEST_PARS
    {
        conv_rst_pars = &reg_signal->test_rst_pars;
        load_pars     = &conv->load_pars_test;
    }

    // Set debug pointer to the RST pars structure that will be initialized

    conv_rst_pars->debug = conv_rst_pars->next;

    // Update the period for the signal - the period is common to operational and test parameters

    reg_signal->reg_period_iters = reg_period_iters;
    reg_signal->reg_period       = conv->iter_period * (double)reg_period_iters;

    // When actuation is CURRENT_REF then don't try to initialize the RST regulation,
    // just prepare the periods so that the delayed_reg calculation works

    if(conv->par_values.global_actuation[0] == REG_CURRENT_REF)
    {
        conv_rst_pars->next->status               = REG_OK;
        conv_rst_pars->next->reg_mode             = REG_CURRENT;
        conv_rst_pars->next->inv_reg_period_iters = 1.0 / (float)reg_period_iters;
        conv_rst_pars->next->reg_period           = reg_signal->reg_period;

        conv_rst_pars->use_next_pars              = 1;
    }
    else // Actuation is VOLTAGE_REF
    {
        struct reg_rst manual;

        // Prepare structure with manual RST coefficients

        memcpy(manual.r, manual_r, REG_N_RST_COEFFS * sizeof(double));  // On Mac, gcc returns an error compiling sizeof(manual_r)
        memcpy(manual.s, manual_s, REG_N_RST_COEFFS * sizeof(double));
        memcpy(manual.t, manual_t, REG_N_RST_COEFFS * sizeof(double));

        // if pure_delay_periods is zero then calculate it

        if(pure_delay_periods <= 0.0)
        {
            pure_delay_periods = (conv->sim_vs_pars.v_ref_delay_iters + conv->sim_vs_pars.vs_delay_iters +
                                  reg_signal->meas.delay_iters[reg_signal->meas.reg_select]) / (float)reg_period_iters;
        }

        // if new RST parameters are valid then request switch of RST parameters by RT thread

        if(regRstInit(conv_rst_pars->next, reg_period_iters, reg_signal->reg_period, load_pars,
                      auxpole1_hz, auxpoles2_hz, auxpoles2_z, auxpole4_hz, auxpole5_hz,
                      pure_delay_periods, track_delay_periods, reg_mode, &manual) != REG_FAULT)
        {
            // conv_rst_pars->use_next_pars can be reset at any time by the real-time thread
            // so the local use_next_pars is returned

            conv_rst_pars->use_next_pars = 1;
        }
    }
}



void regConvPars(struct reg_conv *conv, uint32_t pars_mask)
{
    uint32_t        i;
    uint32_t        flags;
    uint32_t        load_select;
    uint32_t        load_test_select;
    uint32_t        test_pars_mask = pars_mask;
    struct reg_par *par;

    // Update load_select and load_test_select if they are supplied by calling program and if they arevalid

    if(conv->pars.load_select.value != REG_PAR_NOT_USED)
    {
        load_select = *((uint32_t*)conv->pars.load_select.value);

        if(load_select < REG_N_LOADS)
        {
            conv->par_values.load_select[0] = load_select;
        }

        if(conv->pars.load_test_select.value != REG_PAR_NOT_USED)
        {
            load_test_select = *((uint32_t*)conv->pars.load_test_select.value);

            if(load_test_select < REG_N_LOADS)
            {
                conv->par_values.load_test_select[0] = load_test_select;
            }
        }
    }

    load_select      = conv->par_values.load_select[0];
    load_test_select = conv->par_values.load_test_select[0];

    // Scan all active parameters for changes

    par = (struct reg_par *)&conv->pars;

    for(i = 0 ; i < REG_N_PARS ; i++, par++)
    {
        if(par->value != REG_PAR_NOT_USED)
        {
            flags = par->flags;

            // Skip parameters that must be ignored or are not relevant

            if((conv->reg_mode     == REG_NONE    || (flags & REG_MODE_NONE_ONLY) == 0) &&
               (conv->b.regulation == REG_ENABLED || (flags & REG_FIELD_REG     ) == 0) &&
               (conv->i.regulation == REG_ENABLED || (flags & REG_CURRENT_REG   ) == 0))
            {
                char *value_p = (char*)par->value;
                size_t size_in_bytes;

                // If parameter is an array based on load select then point to scalar value addressed by load_select

                if((flags & REG_LOAD_SELECT) != 0)
                {
                    size_in_bytes = par->sizeof_type;

                    value_p += load_select * size_in_bytes;
                }
                else
                {
                    size_in_bytes = par->size_in_bytes;
                }

                // If parameter value has changed

                if(memcmp(par->copy_of_value,value_p,size_in_bytes) != 0)
                {
                    // Save the changed value and set flags for this parameter

                    memcpy(par->copy_of_value,value_p,size_in_bytes);

                    pars_mask |= flags;
                }

                // If parameter is an array based on load select then point to scalar value addressed by load_test_select

                if((flags & (REG_LOAD_SELECT|REG_TEST_PAR|REG_MODE_NONE_ONLY)) == (REG_LOAD_SELECT|REG_TEST_PAR))
                {
                    value_p = (char*)par->value + load_test_select * size_in_bytes;

                    // If parameter value has changed

                    if(memcmp(par->copy_of_value,value_p,size_in_bytes) != 0)
                    {
                        // Save the changed value and set flags for this parameter

                        memcpy(par->copy_of_value,value_p,size_in_bytes);

                        test_pars_mask |= flags;
                    }
                }
            }
        }
    }

    // Check every parameter flag in hierarchical order

    // REG_PAR_SIM_VS

    if((pars_mask & REG_PAR_SIM_VS) != 0)
    {
        regSimVsInit(          &conv->sim_vs_pars,
                                conv->iter_period,
                                conv->par_values.vs_v_ref_delay_iters[0],
                                conv->par_values.vs_bandwidth[0],
                                conv->par_values.vs_z[0],
                                conv->par_values.vs_tau_zero[0],
                                conv->par_values.vs_sim_den,
                                conv->par_values.vs_sim_num);
    }

    // REG_PAR_INVERT_LIMITS

    if((pars_mask & REG_PAR_INVERT_LIMITS) != 0)
    {
        regLimMeasInvert(      &conv->i.lim_meas,
                                conv->par_values.limits_invert[0]);
        regLimMeasInvert(      &conv->b.lim_meas,
                                conv->par_values.limits_invert[0]);
        regLimRefInvert(       &conv->b.lim_ref,
                                conv->par_values.limits_invert[0]);
        regLimRefInvert(       &conv->i.lim_ref,
                                conv->par_values.limits_invert[0]);
        regLimRefInvert(       &conv->v.lim_ref,
                                conv->par_values.limits_invert[0]);
}

    // REG_PAR_V_LIMITS_REF

    if((pars_mask & REG_PAR_V_LIMITS_REF) != 0)
    {
        regLimVrefInit(        &conv->v.lim_ref,
                                conv->par_values.limits_v_pos[0],
                                conv->par_values.limits_v_neg[0],
                                conv->par_values.limits_v_rate[0],
                                conv->par_values.limits_i_quadrants41,
                                conv->par_values.limits_v_quadrants41);
    }

    // REG_PAR_V_LIMITS_ERR

    if((pars_mask & REG_PAR_V_LIMITS_ERR) != 0)
    {
        regErrInitLimits(      &conv->v.err,
                                conv->par_values.limits_v_err_warning[0],
                                conv->par_values.limits_v_err_fault[0]);
    }

    // REG_PAR_I_LIMITS_MEAS (reg mode NONE only)

    if((pars_mask & REG_PAR_I_LIMITS_MEAS) != 0)
    {
        regLimMeasInit(        &conv->i.lim_meas,
                                conv->par_values.limits_i_pos[0],
                                conv->par_values.limits_i_neg[0],
                                conv->par_values.limits_i_pos[0] * conv->par_values.limits_i_low_div_pos[0],
                                conv->par_values.limits_i_pos[0] * conv->par_values.limits_i_zero_div_pos[0]);
    }

    // REG_PAR_I_LIMITS_REF

    if((pars_mask & REG_PAR_I_LIMITS_REF) != 0)
    {
        regLimRefInit (        &conv->i.lim_ref,
                                conv->par_values.limits_i_pos[0],
                                conv->par_values.limits_i_min[0],
                                conv->par_values.limits_i_neg[0],
                                conv->par_values.limits_i_rate[0],
                                conv->par_values.limits_i_closeloop[0]);
    }

    // REG_PAR_I_LIMITS_ERR

    if((pars_mask & REG_PAR_I_LIMITS_ERR) != 0)
    {
        regErrInitLimits(      &conv->i.err,
                                conv->par_values.limits_i_err_warning[0],
                                conv->par_values.limits_i_err_fault[0]);
    }

    // REG_PAR_B_LIMITS_MEAS (reg mode NONE only)

    if((pars_mask & REG_PAR_B_LIMITS_MEAS) != 0)
    {
        regLimMeasInit(        &conv->b.lim_meas,
                                conv->par_values.limits_b_pos[0],
                                conv->par_values.limits_b_neg[0],
                                conv->par_values.limits_b_pos[0] * conv->par_values.limits_b_low_div_pos[0],
                                conv->par_values.limits_b_pos[0] * conv->par_values.limits_b_zero_div_pos[0]);
    }

    // REG_PAR_B_LIMITS_REF

    if((pars_mask & REG_PAR_B_LIMITS_REF) != 0)
    {
        regLimRefInit (        &conv->b.lim_ref,
                                conv->par_values.limits_b_pos[0],
                                conv->par_values.limits_b_min[0],
                                conv->par_values.limits_b_neg[0],
                                conv->par_values.limits_b_rate[0],
                                conv->par_values.limits_b_closeloop[0]);
    }

    // REG_PAR_B_LIMITS_ERR

    if((pars_mask & REG_PAR_B_LIMITS_ERR) != 0)
    {
        regErrInitLimits(      &conv->b.err,
                                conv->par_values.limits_b_err_warning[0],
                                conv->par_values.limits_b_err_fault[0]);
    }

    // REG_PAR_I_LIMITS_RMS

    if((pars_mask & REG_PAR_I_LIMITS_RMS) != 0)
    {
        regLimRmsInit(         &conv->lim_i_rms,
                                conv->par_values.limits_i_rms_warning[0],
                                conv->par_values.limits_i_rms_fault[0],
                                conv->par_values.limits_i_rms_tc[0],
                                conv->iter_period);
    }

    // REG_PAR_I_LIMITS_RMS_LOAD

    if((pars_mask & REG_PAR_I_LIMITS_RMS_LOAD) != 0)
    {
        regLimRmsInit(         &conv->lim_i_rms_load,
                                conv->par_values.limits_i_rms_load_warning[0],
                                conv->par_values.limits_i_rms_load_fault[0],
                                conv->par_values.limits_i_rms_load_tc[0],
                                conv->iter_period);
    }

    // REG_PAR_I_MEAS_REG_SELECT

    if((pars_mask & REG_PAR_I_MEAS_REG_SELECT) != 0)
    {
        conv->i.meas.reg_select = conv->par_values.meas_i_reg_select[0];
    }

    // REG_PAR_I_MEAS_FILTER (reg mode NONE only)

    if((pars_mask & REG_PAR_I_MEAS_FILTER) != 0)
    {
        regMeasFilterInit(     &conv->i.meas,
                                conv->par_values.meas_i_fir_lengths,
                                conv->par_values.ireg_period_iters[0],
                                conv->par_values.limits_i_pos[0],
                                conv->par_values.limits_i_neg[0],
                                conv->par_values.meas_i_delay_iters[0]);
    }

    // REG_PAR_B_MEAS_REG_SELECT

    if((pars_mask & REG_PAR_B_MEAS_REG_SELECT) != 0)
    {
        conv->b.meas.reg_select = conv->par_values.meas_b_reg_select[0];
    }

    // REG_PAR_B_MEAS_FILTER (reg mode NONE only and field regulation ENABLED)

    if((pars_mask & REG_PAR_B_MEAS_FILTER) != 0 && conv->b.regulation == REG_ENABLED)
    {
        regMeasFilterInit(     &conv->b.meas,
                                conv->par_values.meas_b_fir_lengths,
                                conv->par_values.breg_period_iters[0],
                                conv->par_values.limits_b_pos[0],
                                conv->par_values.limits_b_neg[0],
                                conv->par_values.meas_b_delay_iters[0]);
    }

    // REG_PAR_MEAS_SIM_DELAYS (reg mode NONE only)

    if((pars_mask & REG_PAR_MEAS_SIM_DELAYS) != 0)
    {
        regDelayInitDelay(     &conv->b.sim.meas_delay,
                                conv->par_values.vs_v_ref_delay_iters[0] + conv->par_values.meas_b_delay_iters[0] - 1.0);

        regDelayInitDelay(     &conv->i.sim.meas_delay,
                                conv->par_values.vs_v_ref_delay_iters[0] + conv->par_values.meas_i_delay_iters[0] - 1.0);

        regDelayInitDelay(     &conv->v.sim.meas_delay,
                                conv->par_values.vs_v_ref_delay_iters[0] + conv->par_values.meas_v_delay_iters[0] - 1.0);
    }

    // REG_PAR_MEAS_SIM_NOISE_AND_TONE

    if((pars_mask & REG_PAR_MEAS_SIM_NOISE_AND_TONE) != 0)
    {
        // Current and field measurement simulations have noise and tone

        regMeasSetNoiseAndTone(&conv->b.sim.noise_and_tone,
                                conv->par_values.meas_b_sim_noise_pp[0],
                                conv->par_values.meas_b_sim_tone_amp[0],
                                conv->par_values.meas_tone_half_period_iters[0]);

        regMeasSetNoiseAndTone(&conv->i.sim.noise_and_tone,
                                conv->par_values.meas_i_sim_noise_pp[0],
                                conv->par_values.meas_i_sim_tone_amp[0],
                                conv->par_values.meas_tone_half_period_iters[0]);

        // Voltage measurement simulation only had noise and no tone

        regMeasSetNoiseAndTone(&conv->v.sim.noise_and_tone,
                                conv->par_values.meas_v_sim_noise_pp[0],
                                0.0,
                                0);
    }

    // REG_PAR_LOAD

    if((pars_mask & REG_PAR_LOAD) != 0)
    {
        regLoadInit(           &conv->load_pars,
                                conv->par_values.load_ohms_ser[0],
                                conv->par_values.load_ohms_par[0],
                                conv->par_values.load_ohms_mag[0],
                                conv->par_values.load_henrys[0],
                                conv->par_values.load_gauss_per_amp[0]);
    }

    // REG_PAR_LOAD_SAT

    if((pars_mask & REG_PAR_LOAD_SAT) != 0)
    {
        regLoadInitSat(        &conv->load_pars,
                                conv->par_values.load_henrys_sat[0],
                                conv->par_values.load_i_sat_start[0],
                                conv->par_values.load_i_sat_end[0]);
    }

    // REG_PAR_LOAD_SIM

    if((pars_mask & REG_PAR_LOAD_SIM) != 0)
    {
        regSimLoadInit(        &conv->sim_load_pars,
                               &conv->load_pars,
                                conv->par_values.load_sim_tc_error[0],
                                conv->iter_period);
    }

    // REG_PAR_IREG

    if((pars_mask & REG_PAR_IREG) != 0)
    {
        // Loop until updated parameters have been accepted. This may take one iteration period
        // before the real-time thread/interrupt executes and processes a pending set of RST parameters

        while(conv->i.op_rst_pars.use_next_pars == 1);

        regConvRstInit(         conv,
                                REG_CURRENT,
                                REG_OPERATIONAL_RST_PARS,
                                conv->par_values.ireg_period_iters[0],
                                conv->par_values.ireg_auxpole1_hz[0],
                                conv->par_values.ireg_auxpoles2_hz[0],
                                conv->par_values.ireg_auxpoles2_z[0],
                                conv->par_values.ireg_auxpole4_hz[0],
                                conv->par_values.ireg_auxpole5_hz[0],
                                conv->par_values.ireg_pure_delay_periods[0],
                                conv->par_values.ireg_track_delay_periods[0],
                                conv->par_values.ireg_r,
                                conv->par_values.ireg_s,
                                conv->par_values.ireg_t);
    }

    // REG_PAR_BREG

    if((pars_mask & REG_PAR_BREG) != 0 && conv->b.regulation == REG_ENABLED)
    {
        // Loop until updated parameters have been accepted. This may take one iteration period
        // before the real-time thread/interrupt executes and processes a pending set of RST parameters

        while(conv->b.op_rst_pars.use_next_pars == 1);

        regConvRstInit(         conv,
                                REG_FIELD,
                                REG_OPERATIONAL_RST_PARS,
                                conv->par_values.breg_period_iters[0],
                                conv->par_values.breg_auxpole1_hz[0],
                                conv->par_values.breg_auxpoles2_hz[0],
                                conv->par_values.breg_auxpoles2_z[0],
                                conv->par_values.breg_auxpole4_hz[0],
                                conv->par_values.breg_auxpole5_hz[0],
                                conv->par_values.breg_pure_delay_periods[0],
                                conv->par_values.breg_track_delay_periods[0],
                                conv->par_values.breg_r,
                                conv->par_values.breg_s,
                                conv->par_values.breg_t);
    }

    // REG_PAR_LOAD_TEST

    if((test_pars_mask & REG_PAR_LOAD_TEST) != 0)
    {
        regLoadInit(           &conv->load_pars_test,
                                conv->par_values.load_ohms_ser[1],
                                conv->par_values.load_ohms_par[1],
                                conv->par_values.load_ohms_mag[1],
                                conv->par_values.load_henrys[1],
                                conv->par_values.load_gauss_per_amp[1]);
    }

    // REG_PAR_LOAD_SAT_TEST

    if((test_pars_mask & REG_PAR_LOAD_SAT_TEST) != 0)
    {
        regLoadInitSat(        &conv->load_pars_test,
                                conv->par_values.load_henrys_sat[1],
                                conv->par_values.load_i_sat_start[1],
                                conv->par_values.load_i_sat_end[1]);
    }

    // REG_PAR_IREG_TEST

    if((test_pars_mask & REG_PAR_IREG_TEST) != 0)
    {
        // Loop until updated parameters have been accepted. This may take one iteration period
        // before the real-time thread/interrupt executes and processes a pending set of RST parameters

        while(conv->i.test_rst_pars.use_next_pars == 1);

        regConvRstInit(         conv,
                                REG_CURRENT,
                                REG_TEST_RST_PARS,
                                conv->par_values.ireg_period_iters[0],
                                conv->par_values.ireg_auxpole1_hz[1],
                                conv->par_values.ireg_auxpoles2_hz[1],
                                conv->par_values.ireg_auxpoles2_z[1],
                                conv->par_values.ireg_auxpole4_hz[1],
                                conv->par_values.ireg_auxpole5_hz[1],
                                conv->par_values.ireg_pure_delay_periods[1],
                                conv->par_values.ireg_track_delay_periods[1],
                                conv->par_values.ireg_test_r,
                                conv->par_values.ireg_test_s,
                                conv->par_values.ireg_test_t);
    }

    // REG_PAR_BREG_TEST

    if((test_pars_mask & REG_PAR_BREG_TEST) != 0 && conv->b.regulation == REG_ENABLED)
    {
        // Loop until updated parameters have been accepted. This may take one iteration period
        // before the real-time thread/interrupt executes and processes a pending set of RST parameters

        while(conv->b.test_rst_pars.use_next_pars == 1);

        regConvRstInit(         conv,
                                REG_FIELD,
                                REG_TEST_RST_PARS,
                                conv->par_values.breg_period_iters[0],
                                conv->par_values.breg_auxpole1_hz[1],
                                conv->par_values.breg_auxpoles2_hz[1],
                                conv->par_values.breg_auxpoles2_z[1],
                                conv->par_values.breg_auxpole4_hz[1],
                                conv->par_values.breg_auxpole5_hz[1],
                                conv->par_values.breg_pure_delay_periods[1],
                                conv->par_values.breg_track_delay_periods[1],
                                conv->par_values.breg_test_r,
                                conv->par_values.breg_test_s,
                                conv->par_values.breg_test_t);
    }
}



void regConvInitSim(struct reg_conv *conv, enum reg_mode reg_mode, float start)
{
    // Initialise all libreg parameters

    regConvPars(conv, 0xFFFFFFFF);

    regConvPrepareSignalRT(conv, REG_FIELD,   0, 0);
    regConvPrepareSignalRT(conv, REG_CURRENT, 0, 0);

    // Initialise load simulation

    switch(reg_mode)
    {
    case REG_NONE:

        regSimLoadSetVoltage(&conv->sim_load_pars, &conv->sim_load_vars, 0.0);
        break;

    case REG_VOLTAGE:

        regSimLoadSetVoltage(&conv->sim_load_pars, &conv->sim_load_vars, start * conv->sim_vs_pars.gain);
        break;

    case REG_CURRENT:

        regSimLoadSetCurrent(&conv->sim_load_pars, &conv->sim_load_vars, start);
        break;

    case REG_FIELD:

        regSimLoadSetField(&conv->sim_load_pars, &conv->sim_load_vars, start);
        break;
    }

    conv->v.meas = conv->sim_load_vars.circuit_voltage;

    conv->i.meas.signal[REG_MEAS_UNFILTERED] = conv->sim_load_vars.circuit_current;
    conv->b.meas.signal[REG_MEAS_UNFILTERED] = conv->sim_load_vars.magnet_field;

    // Initialize simulated measurement delay histories

    regDelayInitVars(&conv->b.sim.meas_delay, conv->b.meas.signal[REG_MEAS_UNFILTERED]);
    regDelayInitVars(&conv->i.sim.meas_delay, conv->i.meas.signal[REG_MEAS_UNFILTERED]);
    regDelayInitVars(&conv->v.sim.meas_delay, conv->v.meas);

    // Initialize current and field (if enabled) filter histories

    regConvPars(conv, conv->b.regulation == REG_ENABLED ? REG_PAR_B_MEAS_FILTER | REG_PAR_I_MEAS_FILTER  : REG_PAR_I_MEAS_FILTER);

    // Initialize voltage source model history according to the actuation because for CURRENT_REF,
    // the model is used for the current in the load rather than the voltage from the voltage source

    if(conv->par_values.global_actuation[0] == REG_VOLTAGE_REF)
    {
        conv->v.ref_sat     =
        conv->v.ref_limited =
        conv->v.ref       = regSimVsInitHistory(&conv->sim_vs_pars, &conv->sim_vs_vars, conv->v.meas);

        if(reg_mode == REG_FIELD)
        {
            conv->ref_openloop = (conv->b.rst_pars->openloop_reverse.act[0] + conv->b.rst_pars->openloop_reverse.act[1]) * conv->v.ref /
                                 (1.0 - conv->b.rst_pars->openloop_reverse.ref[1]);

            regRstInitHistory(&conv->b.rst_vars, start, conv->ref_openloop, conv->v.ref);
        }
        else if(reg_mode == REG_CURRENT)
        {
            conv->ref_openloop = (conv->i.rst_pars->openloop_reverse.act[0] + conv->i.rst_pars->openloop_reverse.act[1]) * conv->v.ref /
                                 (1.0 - conv->i.rst_pars->openloop_reverse.ref[1]);

            regRstInitHistory(&conv->i.rst_vars, start, conv->ref_openloop, conv->v.ref);
        }

        conv->meas = conv->ref = conv->ref_limited = conv->ref_rst = start;
    }
    else // Actuation is CURRENT_REF
    {
        conv->ref_limited = start;
        conv->sim_load_vars.magnet_current = conv->ref_limited * conv->sim_vs_pars.gain;

        regSimVsInitHistory(&conv->sim_vs_pars, &conv->sim_vs_vars, conv->sim_load_vars.magnet_current);
    }

    regConvPrepareSignalRT(conv, reg_mode, 0, 0);
    regConvSetModeRT(conv, reg_mode);
    regConvSimulateRT(conv, 0.0);
}



void regConvInitMeas(struct reg_conv *conv, struct reg_meas_signal *v_meas_p, struct reg_meas_signal *i_meas_p, struct reg_meas_signal *b_meas_p)
{
    static struct reg_meas_signal null_signal = { 0.0, REG_MEAS_SIGNAL_OK };

    conv->b.input_p = (b_meas_p == NULL ? &null_signal : b_meas_p);;
    conv->i.input_p = (i_meas_p == NULL ? &null_signal : i_meas_p);;
    conv->v.input_p = (v_meas_p == NULL ? &null_signal : v_meas_p);
}



// Real-Time Functions

/*!
 * Function to prepare real-time processing each iteration for a regulation signal (Field or Current)
 *
 * This function is called to check if the RST coefficient have been updated by the non-real-time thread
 * and is also able to set the iteration period when the reg_mode is NONE. This allows synchronous
 * regulation by multiple systems.
 *
 * @param[in,out]     conv        Pointer to converter regulation structure
 * @param[in]         reg_mode    Regulation signal to process (REG_FIELD or REG_CURRENT)
 * @param[in]         unix_time   Unix_time for this iteration
 * @param[in]         us_time     Microsecond time for this iteration
 */
static void regConvPrepareSignalRT(struct reg_conv *conv, enum reg_mode reg_mode, uint32_t unix_time, uint32_t us_time)
{
    struct reg_rst_pars    *rst_pars;
    struct reg_conv_signal *reg_signal = reg_mode == REG_FIELD ? &conv->b : &conv->i;

    // If the option of regulation for this signal is enabled

    if(reg_signal->regulation == REG_ENABLED)
    {
        // Switch operation RST parameter pointers when switch flag is active

        if(reg_signal->op_rst_pars.use_next_pars != 0)
        {
            rst_pars                       = reg_signal->op_rst_pars.next;
            reg_signal->op_rst_pars.next   = reg_signal->op_rst_pars.active;
            reg_signal->op_rst_pars.active = rst_pars;
            reg_signal->op_rst_pars.use_next_pars = 0;
        }

        // Switch test RST parameter pointers when switch flag is active

        if(reg_signal->test_rst_pars.use_next_pars != 0)
        {
            rst_pars                         = reg_signal->test_rst_pars.next;
            reg_signal->test_rst_pars.next   = reg_signal->test_rst_pars.active;
            reg_signal->test_rst_pars.active = rst_pars;
            reg_signal->test_rst_pars.use_next_pars = 0;
        }

        // Set rst_pars pointer to link to the active RST parameters (operational or test)

        reg_signal->rst_pars = conv->reg_rst_source == REG_OPERATIONAL_RST_PARS ? reg_signal->op_rst_pars.active : reg_signal->test_rst_pars.active;

        // Increment iteration counter for this new iteration

        reg_signal->iteration_counter++;

        // Set regulation iteration counter from the time when reg_mode is NONE

        if(conv->reg_mode == REG_NONE)
        {
            /*!
             * If multiple systems must synchronize their regulation periods, then the regulation
             * period (reg_period_iters * iter_period_us) should divide into 12s exactly.
             * This awk script lists the periods in microseconds from 100us to 10000us (10kHz to 10Hz) that do this:
             *     awk 'BEGIN { for(us=100 ; us<=100000 ; us++) { g = 12000000/us; if (int(g) == g) print us}}'
             */

            reg_signal->iteration_counter = ((((unix_time % 12) * 1000000) + us_time) / conv->iter_period_us) % reg_signal->reg_period_iters;
        }
    }
}



static void regConvSetModeNoneOrVoltageRT(struct reg_conv *conv, enum reg_mode reg_mode)
{
    if(reg_mode == REG_VOLTAGE)
    {
        // Voltage regulation - Initialize voltage references according to previous regulation mode

        switch(conv->reg_mode)
        {
            case REG_FIELD:

                conv->v.ref     = regRstAverageVrefRT(&conv->b.rst_vars);
                conv->v.ref_sat = conv->v.ref;

                regErrResetLimitsVarsRT(&conv->b.err, 0);
                break;

            case REG_CURRENT:

                conv->v.ref     = regRstAverageVrefRT(&conv->i.rst_vars);
                conv->v.ref_sat = regLoadVrefSatRT(&conv->load_pars, conv->i.rst_vars.meas[0], conv->v.ref);

                regErrResetLimitsVarsRT(&conv->i.err, 0);
                break;

            default:    // NONE

                conv->v.ref_sat = conv->v.ref;
                regErrResetLimitsVarsRT(&conv->i.err, REG_INHIBIT_MAX_ABS_ERR_ITERATIONS);
                break;
        }

        conv->v.ref_limited = conv->v.ref_sat;
    }
    else // REG_NONE
    {
        conv->v.ref         = 0.0;
        conv->v.ref_sat     = 0.0;
        conv->v.ref_limited = 0.0;

        regErrResetLimitsVarsRT(&conv->b.err, 0);
        regErrResetLimitsVarsRT(&conv->i.err, 0);
        regErrResetLimitsVarsRT(&conv->v.err, 0);
    }

    // Clear field and current regulation variables

    conv->meas                = 0.0;
    conv->ref                 = 0.0;
    conv->ref_limited         = 0.0;
    conv->ref_rst             = 0.0;
    conv->ref_openloop        = 0.0;
    conv->ref_delayed         = 0.0;
    conv->track_delay_periods = 0.0;

    conv->flags.ref_clip      = 0;
    conv->flags.ref_rate      = 0;

    conv->reg_signal = &conv->i;
    conv->lim_ref    = &conv->v.lim_ref;

    conv->ref_advance = conv->iter_period * (conv->sim_vs_pars.v_ref_delay_iters + conv->sim_vs_pars.vs_delay_iters);
}



static void regConvSetModeFieldOrCurrentRT(struct reg_conv *conv, enum reg_mode reg_mode)
{
    uint32_t                idx;
    struct reg_conv_signal *reg_signal;
    struct reg_rst_pars    *rst_pars;
    struct reg_rst_vars    *rst_vars;

    // Get points to RST parameters and variables

    reg_signal = conv->reg_signal = (reg_mode == REG_FIELD ? &conv->b : &conv->i);

    rst_pars         =  reg_signal->rst_pars;
    rst_vars         = &reg_signal->rst_vars;
    conv->lim_ref    = &reg_signal->lim_ref;
    conv->reg_period =  rst_pars->reg_period;

    // If actuation is CURRENT_REF then current regulation is open-loop in libreg

    if(conv->par_values.global_actuation[0] == REG_CURRENT_REF)
    {
        regConvSetModeNoneOrVoltageRT(conv, REG_NONE);

        rst_pars->ref_delay_periods = conv->ref_advance / conv->iter_period;

        conv->meas = reg_signal->meas.signal[reg_signal->meas.reg_select];
        conv->ref  = conv->ref_delayed = conv->meas;

        for(idx = 0; idx <= REG_RST_HISTORY_MASK; idx++)
        {
            rst_vars->act         [idx] = 0.0;
            rst_vars->meas        [idx] = conv->meas;
            rst_vars->ref         [idx] = conv->meas;
            rst_vars->openloop_ref[idx] = conv->meas;
        }
    }
    else // Actuation is VOLTAGE_REF
    {
        conv->ref_advance = rst_pars->track_delay_periods * rst_pars->reg_period - reg_signal->meas.delay_iters[reg_signal->meas.reg_select] * conv->iter_period;

        rst_pars->ref_delay_periods = rst_pars->track_delay_periods +
                                     (reg_signal->meas.delay_iters[REG_MEAS_FILTERED] - reg_signal->meas.delay_iters[reg_signal->meas.reg_select]) /
                                     (float)reg_signal->reg_period_iters;

        regRstInitRefRT(rst_pars, rst_vars, reg_signal->rate.estimate);

        conv->ref = regRstPrevRefRT(&reg_signal->rst_vars);

        conv->ref_delayed = regRstDelayedRefRT(rst_pars, rst_vars, reg_signal->iteration_counter);

        regErrResetLimitsVarsRT(&reg_signal->err, REG_INHIBIT_MAX_ABS_ERR_ITERATIONS);
    }

    conv->ref_limited = conv->ref_rst = conv->ref;
}



uint32_t regConvSetMeasRT(struct reg_conv *conv, enum reg_rst_source reg_rst_source, uint32_t unix_time, uint32_t us_time, uint32_t use_sim_meas)
{
    uint32_t iteration_counter;
    float    i_meas_unfiltered;

    // Receive reg_rst_source for this iteration

    conv->reg_rst_source = reg_rst_source;

    // Check for new RST parameters and manage iteration counters

    regConvPrepareSignalRT(conv, REG_FIELD,   unix_time, us_time);
    regConvPrepareSignalRT(conv, REG_CURRENT, unix_time, us_time);

    // Use simulated or real measurements as required

    if(use_sim_meas == 0)
    {
        // Use real field, current and voltage measurements and measurement statuses supplied by application

        conv->b.input = *conv->b.input_p;
        conv->i.input = *conv->i.input_p;
        conv->v.input = *conv->v.input_p;
    }
    else
    {
        // Use simulated measurements which are always OK

        conv->b.input.signal = conv->b.sim.signal;
        conv->i.input.signal = conv->i.sim.signal;
        conv->v.input.signal = conv->v.sim.signal;

        conv->b.input.status = REG_MEAS_SIGNAL_OK;
        conv->i.input.status = REG_MEAS_SIGNAL_OK;
        conv->v.input.status = REG_MEAS_SIGNAL_OK;
    }

    // If regulating CURRENT or FIELD then calculate the delayed reference

    if(conv->reg_mode == REG_CURRENT || conv->reg_mode == REG_FIELD)
    {
        // Note that regRstDelayedRefRT requires the iteration counter to run from 1 - reg_period_iters so it is
        // used after incrementing in regConvSignalRT() and before being wrapped

        conv->ref_delayed = regRstDelayedRefRT(conv->reg_signal->rst_pars, &conv->reg_signal->rst_vars, conv->reg_signal->iteration_counter);
    }

    // Wrap iteration counters

    if(conv->i.iteration_counter >= conv->i.reg_period_iters)
    {
        conv->i.iteration_counter = 0;
    }

    if(conv->b.iteration_counter >= conv->b.reg_period_iters)
    {
        conv->b.iteration_counter = 0;
    }

    // Prepare to return the iteration counter for the active regulation mode

    if(conv->reg_mode == REG_CURRENT)
    {
        iteration_counter = conv->i.iteration_counter;
    }
    else if(conv->reg_mode == REG_FIELD)
    {
        iteration_counter = conv->b.iteration_counter;
    }
    else // REG_NONE or REG_VOLTAGE
    {
        iteration_counter = 0;
    }

    // Check voltage measurement

    if(conv->v.input.status != REG_MEAS_SIGNAL_OK)
    {
        // If input voltage measurement is invalid then use voltage source model instead

        conv->v.meas = conv->v.err.delayed_ref;
        conv->v.invalid_input_counter++;
    }
    else
    {
        conv->v.meas = conv->v.input.signal;
    }

    // Check current measurement

    if(conv->i.input.status != REG_MEAS_SIGNAL_OK)
    {
        if(conv->reg_mode == REG_CURRENT)
        {
            // If regulating current then use delayed ref adjusted by the regulation error as the measurement

            i_meas_unfiltered = conv->i.meas.signal[REG_MEAS_UNFILTERED] = conv->ref_delayed - conv->i.err.err;
        }
        else
        {
            // If not regulating current then extrapolate previous value using the current rate of change

            i_meas_unfiltered = conv->i.meas.signal[REG_MEAS_UNFILTERED] += conv->i.rate.estimate * conv->iter_period;
        }

        conv->i.invalid_input_counter++;
    }
    else
    {
        i_meas_unfiltered = conv->i.meas.signal[REG_MEAS_UNFILTERED] = conv->i.input.signal;
    }

    // Filter the current measurement and prepare to estimate the measurement rate

    regMeasFilterRT(&conv->i.meas);
    regMeasRateRT  (&conv->i.rate, conv->i.meas.signal[REG_MEAS_FILTERED],
                     conv->i.rst_pars->reg_period, conv->i.reg_period_iters);

    // Check current measurement and RMS limits

    regLimMeasRT   (&conv->i.lim_meas,     i_meas_unfiltered);
    regLimMeasRmsRT(&conv->lim_i_rms,      i_meas_unfiltered);
    regLimMeasRmsRT(&conv->lim_i_rms_load, i_meas_unfiltered);

    // Update RST history index and store new measurement

    if(conv->i.iteration_counter == 0)
    {
        regRstIncHistoryIndexRT(&conv->i.rst_vars);
        conv->i.rst_vars.meas[conv->i.rst_vars.history_index] = conv->i.meas.signal[conv->i.meas.reg_select];
    }

    // Check field measurement if option of field regulation is ENABLED

    if(conv->b.regulation == REG_ENABLED)
    {
        if(conv->b.input.status != REG_MEAS_SIGNAL_OK)
        {
            if(conv->reg_mode == REG_FIELD)
            {
                // If regulating field then use delayed ref adjusted by the regulation error as the measurement

                conv->b.meas.signal[REG_MEAS_UNFILTERED] = conv->ref_delayed - conv->b.err.err;
            }
            else
            {
                // If not regulating current then extrapolate previous value using the current rate of change

                conv->b.meas.signal[REG_MEAS_UNFILTERED] += conv->b.rate.estimate * conv->iter_period;
            }

            conv->b.invalid_input_counter++;
        }
        else
        {
            conv->b.meas.signal[REG_MEAS_UNFILTERED] = conv->b.input.signal;
        }

        // Filter the field measurement, prepare to estimate measurement rate and apply limits

        regMeasFilterRT(&conv->b.meas);
        regMeasRateRT  (&conv->b.rate,conv->b.meas.signal[REG_MEAS_FILTERED],
                         conv->b.rst_pars->reg_period,conv->b.reg_period_iters);
        regLimMeasRT   (&conv->b.lim_meas, conv->b.meas.signal[REG_MEAS_UNFILTERED]);

        // Update RST history index and store new measurement

        if(conv->b.iteration_counter == 0)
        {
            regRstIncHistoryIndexRT(&conv->b.rst_vars);

            conv->b.rst_vars.meas[conv->b.rst_vars.history_index] = conv->b.meas.signal[conv->b.meas.reg_select];
        }
    }

    // When actuation is VOLTAGE_REF and the converter is running, then manage the voltage related limits

    if(conv->par_values.global_actuation[0] == REG_VOLTAGE_REF && conv->reg_mode != REG_NONE)
    {
        // Calculate and check the voltage regulation limits

        regErrCheckLimitsRT(&conv->v.err, conv->v.err.delayed_ref, conv->v.meas);

        // Calculate voltage reference limits for the measured current (V limits can depend on current)

        regLimVrefCalcRT(&conv->v.lim_ref, i_meas_unfiltered);
    }

    return(iteration_counter);
}



void regConvSetModeRT(struct reg_conv *conv, enum reg_mode reg_mode)
{
    // If regulation mode has changed

    if(reg_mode != conv->reg_mode)
    {
        switch(reg_mode)
        {
            case REG_NONE:
            case REG_VOLTAGE:

                regConvSetModeNoneOrVoltageRT(conv, reg_mode);
                break;

            case REG_CURRENT:
            case REG_FIELD:

                regConvSetModeFieldOrCurrentRT(conv, reg_mode);
                break;
        }

        // Store the new regulation mode

        conv->reg_mode = reg_mode;
    }
}



static void regConvRegulateSignalRT(struct reg_conv *conv, enum reg_mode reg_mode, float *ref)
{
    struct reg_conv_signal *reg_signal = reg_mode == REG_FIELD ? &conv->b : &conv->i;

    if(reg_signal->regulation == REG_ENABLED)
    {
        // If voltage reference was already calculated, ref will equal NULL

        if(ref == NULL)
        {
            // Save voltage reference in RST actuation history - compensate for magnet saturation for CURRENT regulation

            if(reg_signal->iteration_counter == 0)
            {
                reg_signal->rst_vars.act[reg_signal->rst_vars.history_index] = reg_mode == REG_CURRENT ?
                        regLoadInverseVrefSatRT(&conv->load_pars, conv->i.meas.signal[REG_MEAS_UNFILTERED], conv->v.ref_limited) :
                        conv->v.ref_limited;
            }
        }
        else
        {
            if(reg_signal->iteration_counter == 0)
            {
                conv->ref  = *ref;
                conv->meas = reg_signal->meas.signal[reg_signal->meas.reg_select];

                // Apply current reference clip and rate limits

                conv->ref_limited = regLimRefRT(&reg_signal->lim_ref, conv->reg_period, conv->ref, conv->ref_limited);

                // If Actuation is VOLTAGE_REF

                if(conv->par_values.global_actuation[0] == REG_VOLTAGE_REF)
                {
                    bool  is_limited;
                    float v_ref;

                    // Calculate voltage reference using RST algorithm

                    conv->v.ref = regRstCalcActRT(reg_signal->rst_pars, &reg_signal->rst_vars, conv->ref_limited, conv->is_openloop);

                    // Calculate magnet saturation compensation when regulating current only

                    conv->v.ref_sat = (reg_mode == REG_CURRENT ? regLoadVrefSatRT(&conv->load_pars, conv->i.meas.signal[REG_MEAS_UNFILTERED], conv->v.ref) : conv->v.ref);

                    // Apply voltage reference clip and rate limits

                    conv->v.ref_limited = regLimRefRT(&conv->v.lim_ref, conv->reg_period, conv->v.ref_sat, conv->v.ref_limited);

                    // If voltage reference has been clipped

                    is_limited = (conv->v.lim_ref.flags.clip || conv->v.lim_ref.flags.rate);

                    if(is_limited)
                    {
                        // Back calculate the new v_ref before the saturation compensation when regulating current only

                        v_ref = (reg_mode == REG_CURRENT ? regLoadInverseVrefSatRT(&conv->load_pars, conv->i.meas.signal[REG_MEAS_UNFILTERED], conv->v.ref_limited) : conv->v.ref_limited);

                        // Mark current reference as rate limited

                        reg_signal->lim_ref.flags.rate = 1;
                    }
                    else
                    {
                        v_ref = conv->v.ref;
                    }

                    conv->flags.ref_clip = reg_signal->lim_ref.flags.clip;
                    conv->flags.ref_rate = reg_signal->lim_ref.flags.rate;

                    conv->track_delay_periods = regRstTrackDelayRT(&conv->reg_signal->rst_vars);

                    // Back calculate new current reference to keep RST histories balanced

                    regRstCalcRefRT(reg_signal->rst_pars, &reg_signal->rst_vars, v_ref, is_limited, conv->is_openloop);

                    conv->ref_rst      = reg_signal->rst_vars.ref         [reg_signal->rst_vars.history_index];
                    conv->ref_openloop = reg_signal->rst_vars.openloop_ref[reg_signal->rst_vars.history_index];

                    // Switch between open/closed loop according to closeloop threshold

                    if(conv->is_openloop)
                    {
                        if(fabs(conv->meas) > reg_signal->lim_ref.closeloop)
                        {
                            conv->is_openloop = false;

                            *ref = conv->ref_limited = conv->ref_rst;
                        }
                        else
                        {
                            *ref = conv->ref_openloop;
                        }
                    }
                    else
                    {
                        if(fabs(conv->meas) < reg_signal->lim_ref.closeloop)
                        {
                            conv->is_openloop = true;

                            *ref = conv->ref_limited = conv->ref_openloop;
                        }
                        else
                        {
                            *ref = conv->ref_rst;
                        }
                    }

                }
                else // Actuation is CURRENT_REF
                {
                    conv->i.rst_vars.ref[conv->i.rst_vars.history_index] = conv->ref_limited;
                }
            }

            // Monitor regulation error using the delayed reference and the filtered measurement

            if(conv->par_values.global_reg_err_rate[0] == REG_ERR_RATE_MEASUREMENT || reg_signal->iteration_counter == 0)
            {
                regErrCheckLimitsRT(&reg_signal->err, conv->ref_delayed, reg_signal->meas.signal[REG_MEAS_FILTERED]);
            }
        }
    }
}



void regConvRegulateRT(struct reg_conv *conv, float *ref)
{
    switch(conv->reg_mode)
    {
    case REG_NONE:

        break;

    case REG_VOLTAGE:

        // *ref is a voltage reference so just apply limits

        conv->v.ref_sat = conv->v.ref = *ref;

        conv->v.ref_limited = regLimRefRT(&conv->v.lim_ref, conv->iter_period, conv->v.ref, conv->v.ref_limited);

        conv->flags.ref_clip = conv->v.lim_ref.flags.clip;
        conv->flags.ref_rate = conv->v.lim_ref.flags.rate;

        *ref = conv->v.ref_limited;

        // Keep RST act history up to date for field and current regulation

        regConvRegulateSignalRT(conv, REG_FIELD,   NULL);
        regConvRegulateSignalRT(conv, REG_CURRENT, NULL);
        break;

    case REG_CURRENT:

        regConvRegulateSignalRT(conv, REG_CURRENT, ref);
        regConvRegulateSignalRT(conv, REG_FIELD,   NULL);
        break;

    case REG_FIELD:

        regConvRegulateSignalRT(conv, REG_FIELD,   ref);
        regConvRegulateSignalRT(conv, REG_CURRENT, NULL);
        break;
    }
}



void regConvSimulateRT(struct reg_conv *conv, float v_perturbation)
{
    float v_circuit;      // Simulated v_circuit without V_REF_DELAY

    // If Actuation is VOLTAGE

    if(conv->par_values.global_actuation[0] == REG_VOLTAGE_REF)
    {
        // Simulate voltage source response to v_ref without taking into account V_REF_DELAY

        v_circuit = regSimVsRT(&conv->sim_vs_pars, &conv->sim_vs_vars, conv->v.ref_limited);

        // Simulate load current and field in response to sim_advanced_v_circuit plus the perturbation

        regSimLoadRT(&conv->sim_load_pars, &conv->sim_load_vars, conv->sim_vs_pars.vs_undersampled_flag, v_circuit + v_perturbation);
    }
    else // Actuation is CURRENT_REF
    {
        // Use the voltage source model as the current source model and assume that all the circuit current passes through the magnet
        // i.e. assume ohms_par is large - if this is not true then the simulation will not be accurate

        conv->sim_load_vars.circuit_current = regSimVsRT(&conv->sim_vs_pars, &conv->sim_vs_vars, conv->ref_limited);

        // Derive the circuit voltage using V = I.R + L(I) dI/dt
        // Note: conv->sim_load_vars.magnet_current contains current from previous iteration so it is used to calculate dI/dt

        conv->sim_load_vars.circuit_voltage = conv->sim_load_vars.circuit_current * conv->sim_load_pars.load_pars.ohms +
                                              conv->sim_load_pars.load_pars.henrys *
                                              regLoadSatFactorRT(&conv->sim_load_pars.load_pars,conv->sim_load_vars.circuit_voltage) *
                                              (conv->sim_load_vars.circuit_current - conv->sim_load_vars.magnet_current) / conv->iter_period;

        conv->sim_load_vars.magnet_current = conv->sim_load_vars.circuit_current;

        // Derive the simulated magnetic field

        conv->sim_load_vars.magnet_field = regLoadCurrentToFieldRT(&conv->sim_load_pars.load_pars, conv->sim_load_vars.magnet_current);
    }

    // Use delays to estimate the measurement of the magnet's field and the circuit's current and voltage

    conv->b.sim.signal = regDelaySignalRT(&conv->b.sim.meas_delay, conv->sim_load_vars.magnet_field,
                                           conv->sim_vs_pars.vs_undersampled_flag && conv->sim_load_pars.load_undersampled_flag);
    conv->i.sim.signal = regDelaySignalRT(&conv->i.sim.meas_delay, conv->sim_load_vars.circuit_current,
                                           conv->sim_vs_pars.vs_undersampled_flag && conv->sim_load_pars.load_undersampled_flag);
    conv->v.sim.signal = regDelaySignalRT(&conv->v.sim.meas_delay, conv->sim_load_vars.circuit_voltage,
                                           conv->sim_vs_pars.vs_undersampled_flag);

    // Store simulated voltage measurement without noise as the delayed ref for the v_err calculation

    conv->v.err.delayed_ref = conv->v.sim.signal;

    // Simulate noise and tone on simulated measurement of the magnet's field and the circuit's current and voltage

    conv->b.sim.signal += regMeasNoiseAndToneRT(&conv->b.sim.noise_and_tone);
    conv->i.sim.signal += regMeasNoiseAndToneRT(&conv->i.sim.noise_and_tone);
    conv->v.sim.signal += regMeasNoiseAndToneRT(&conv->v.sim.noise_and_tone);
}

// EOF

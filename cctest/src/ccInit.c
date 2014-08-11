/*---------------------------------------------------------------------------------------------------------*\
  File:     ccInit.c                                                                    Copyright CERN 2014

  License:  This file is part of cctest.

            cctest is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  cctest simulation initialisation functions

  Authors:  Quentin.King@cern.ch

\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Include cctest program header files

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRef.h"
#include "ccSigs.h"
#include "ccRun.h"

/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccInitRun(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called when the RUN command is executed to prepare for the new run.  It checks that
  parameters are valid.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t       i;
    uint32_t       num_reg_modes;
    struct cccmds *cmd;
    static struct reg_meas_signal invalid_meas = { 0.0, REG_MEAS_SIGNAL_INVALID };


    // Set iteration period into libreg structure

    regConvInit(&conv, (double)ccpars_global.iter_period, ccpars_global.actuation);

    // Prepare an invalid signal to allow recovery from invalid signals to be tested

    regConvInitMeas(&conv, &invalid_meas, &invalid_meas, &invalid_meas);

    // Check that GLOBAL FUNCTION and REG_MODE parameters have the same number of elements

    ccrun.func_idx      = 0;
    ccrun.num_functions = global_pars[GLOBAL_FUNCTION].num_elements;
    num_reg_modes       = global_pars[GLOBAL_REG_MODE].num_elements;

    if(num_reg_modes > ccrun.num_functions)
    {
        ccTestPrintError("GLOBAL REG_MODE must not have more elements than GLOBAL FUNCTION");
        return(EXIT_FAILURE);
    }

    // Reset TEST and TRIM types since only one function of each can be armed

    ccpars_test.config.type = FG_TEST_UNDEFINED;
    ccpars_trim.config.type = FG_TRIM_UNDEFINED;

    // Examine the reg_mode for all functions and set flags to indicate which modes are used

    ccrun.breg_flag = 0;
    ccrun.ireg_flag = 0;

    for(i = 0 ; i < ccrun.num_functions ; i++)
    {
        // Extend REG_MODES array to use the last value for any additional functions

        if(i > 0 && i >= num_reg_modes)
        {
            ccpars_global.reg_mode[i] = ccpars_global.reg_mode[i-1];
        }

        // Check that reg_mode is compatible with actuation

        if(conv.actuation == REG_CURRENT_REF && ccpars_global.reg_mode[i] != REG_CURRENT)
        {
            ccTestPrintError("GLOBAL REG_MODE must CURRENT when GLOBAL ACTUATION is CURRENT");
            return(EXIT_FAILURE);
        }

        // Set the field or current regulation flags

        switch(ccpars_global.reg_mode[i])
        {
            case REG_FIELD:   ccrun.breg_flag = 1;       break;
            case REG_CURRENT: ccrun.ireg_flag = 1;       break;
        }
    }

    // if GLOBAL REVERSE_TIME is ENALBED

    if(ccpars_global.reverse_time == CC_ENABLED)
    {
        // SIM_LOAD must be DISABLED
        if(ccpars_global.sim_load == CC_ENABLED)
        {
            ccTestPrintError("GLOBAL SIM_LOAD must be DISABLED when REVERSE_TIME is ENABLED");
            return(EXIT_FAILURE);
        }

        // Only one function can be specified

        if(ccrun.num_functions > 1)
        {
            ccTestPrintError("only one function can be specified when REVERSE_TIME is ENABLED");
            return(EXIT_FAILURE);
        }
    }

    // Check that if REG_MODE is FIELD or CURRENT, SIM_LOAD is ENABLED

    if((ccrun.breg_flag == 1 || ccrun.ireg_flag == 1) && ccpars_global.sim_load != CC_ENABLED)
    {
        ccTestPrintError("GLOBAL SIM_LOAD must be ENABLED if REG_MODE is FIELD or CURRENT");
        return(EXIT_FAILURE);
    }

    // Check if GLOBAL ABORT_TIME is specified that FG_LIMITS is ENABLED

    if(ccpars_global.abort_time > 0.0 && ccpars_global.fg_limits == CC_DISABLED)
    {
        ccTestPrintError("GLOBAL FG_LIMITS must be ENABLED if ABORT_TIME is defined");
        return(EXIT_FAILURE);
    }

    // If voltage perturbation is not required then set perturb_time to far beyond end of simulation

    if(ccpars_load.perturb_time <= 0.0 || ccpars_load.perturb_volts == 0.0)
    {
        ccpars_load.perturb_volts = 0.0;
        ccpars_load.perturb_time  = 1.0E30;
    }

    // Reset enabled flag for all commands (it is used in ccsigsFlot())

    for(cmd = cmds ; cmd->name != NULL ; cmd++)
    {
        cmd->enabled = 0;
    }

    // Enable the commands whose parameters should be included in FLOT colorbox pop-up

    cmds[CMD_GLOBAL].enabled = 1;
    cmds[CMD_LIMITS].enabled = 1;

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        cmds[CMD_GLOBAL].enabled = 1;
        cmds[CMD_LOAD  ].enabled = 1;
        cmds[CMD_MEAS  ].enabled = 1;
        cmds[CMD_VS    ].enabled = 1;
        cmds[CMD_BREG  ].enabled = ccrun.breg_flag;
        cmds[CMD_IREG  ].enabled = ccrun.ireg_flag;
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccInitLoad(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    // If load needs to be simulated

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        // Initialize load model structure

        regLoadInit(&conv.load_pars, ccpars_load.ohms_ser, ccpars_load.ohms_par,
                     ccpars_load.ohms_mag, ccpars_load.henrys, ccpars_load.gauss_per_amp);

        // Initialize load saturation model

        regLoadInitSat(&conv.load_pars, ccpars_load.henrys_sat, ccpars_load.i_sat_start, ccpars_load.i_sat_end);

        // Field regulation requires an inductive load

        if(ccrun.breg_flag == 1 && conv.iter_period > (3.0 * conv.load_pars.tc))
        {
            ccTestPrintError("REG_MODE of FIELD is not permitted for a resistive circuit "
                             "(circuit time constant is less than 1/3 x iteration period)");
            return(EXIT_FAILURE);
        }
    }
    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccInitLimits(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    // If load simulation enabled then limits must be supplied

    if(ccpars_global.sim_load == CC_ENABLED)
    {
         // Initialize field and current measurement trip/low/zero limits

        regLimMeasInit(&conv.b.lim_meas,
                       ccpars_limits.b.pos, ccpars_limits.b.neg,
                       ccpars_limits.b.pos * LOW_MEAS_FACTOR,
                       ccpars_limits.b.pos * ZERO_MEAS_FACTOR,
                       ccpars_limits.invert_limits);

        regLimMeasInit(&conv.i.lim_meas,
                       ccpars_limits.i.pos, ccpars_limits.i.neg,
                       ccpars_limits.i.pos * LOW_MEAS_FACTOR,
                       ccpars_limits.i.pos * ZERO_MEAS_FACTOR,
                       ccpars_limits.invert_limits);

        // Initialize RMS current limits

        regLimMeasRmsInit(&conv.i.lim_meas,
                          ccpars_limits.i_rms_fault,
                          ccpars_limits.i_rms_warning,
                          ccpars_limits.i_rms_tc,
                          conv.iter_period);

        // Initialize field, current and voltage reference pos/min/neg/rate limits

        regLimRefInit (&conv.b.lim_ref, ccpars_limits.b.pos, ccpars_limits.b.neg, ccpars_limits.b.rate,
                       ccpars_limits.invert_limits);

        regLimRefInit (&conv.i.lim_ref, ccpars_limits.i.pos, ccpars_limits.i.neg, ccpars_limits.i.rate,
                       ccpars_limits.invert_limits);

        regLimVrefInit(&conv.v.lim_ref, ccpars_limits.v.pos, ccpars_limits.v.neg, ccpars_limits.v.rate,
                       ccpars_limits.i_quadrants41, ccpars_limits.v_quadrants41, ccpars_limits.invert_limits);

        // Initialize field, current and voltage regulation error warning/fault thresholds

        regErrInitLimits(&conv.b.err, ccpars_limits.b_err_warning, ccpars_limits.b_err_fault);

        regErrInitLimits(&conv.i.err, ccpars_limits.i_err_warning, ccpars_limits.i_err_fault);

        regErrInitLimits(&conv.v.err, ccpars_limits.v_err_warning, ccpars_limits.v_err_fault);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccInitFunctions(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t         func_idx;
    uint32_t         exit_status;
    struct fgfunc   *func;

    // Reset function generation limits pointer by default

    ccrun.fg_limits = NULL;

    // Initialize v_ref limits in case ccrefCheckConverterLimits() is used

    regLimVrefInit(&ccrun.fg_lim_v_ref, ccpars_limits.v.pos, ccpars_limits.v.neg,
                   ccpars_limits.v.rate, ccpars_limits.i_quadrants41, ccpars_limits.v_quadrants41,
                   CC_DISABLED);

    // Initialize the reference functions

    for(func_idx = 0, exit_status = EXIT_SUCCESS ;
        func_idx < ccrun.num_functions && exit_status == EXIT_SUCCESS;
        func_idx++)
    {
        // Initialize pointer to function generation limits when FG_LIMITS is ENABLED

        if(ccpars_global.fg_limits == CC_ENABLED)
        {
            switch(ccpars_global.reg_mode[func_idx])
            {
                case REG_FIELD:   ccrun.fg_limits = &ccpars_limits.b;       break;
                case REG_CURRENT: ccrun.fg_limits = &ccpars_limits.i;       break;
                case REG_VOLTAGE: ccrun.fg_limits = &ccpars_limits.v;       break;
            }

            ccrun.fg_limits->user_check_limits = ccRefCheckConverterLimits;
            ccrun.fg_limits->user_data = ccpars_global.reg_mode[func_idx];
        }

        // Try to arm the next function

        func = &funcs[ccpars_global.function[func_idx]];

        exit_status = func->init_func(&ccrun.fg_meta[func_idx]);

        // Mark command for this function as enabled to include parameters in FLOT colorbox pop-up

        cmds[func->cmd_idx].enabled = 1;
    }

    return(exit_status);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccInitSimulation(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    // By default use voltage source simulation transfer function directly

    conv.sim_vs_pars = ccpars_vs.sim_vs_pars;

    // If VS BANDWIDTH is greater than zero then initialize the voltage source simulation model coefficients
    // using Tustin algorithm.

    regSimVsInitTustin(&conv.sim_vs_pars, conv.iter_period, ccpars_vs.bandwidth, ccpars_vs.z, ccpars_vs.tau_zero);

    // Initialize voltage source model gain and stop if gain error is more than 5%
    // This also calculates the vs_undersampled_flag in sim_vs_pars

    regSimVsInit(&conv.sim_vs_pars, &conv.sim_vs_vars, ccpars_vs.v_ref_delay_iters);

    if(fabs(conv.sim_vs_pars.gain - 1.0) > 0.05)
    {
        ccTestPrintError("voltage source model gain (%.3f) has an error of more than 5%%", conv.sim_vs_pars.gain);
        return(EXIT_FAILURE);
    }

    // First set the V/I/B measurements to cover all three regulation modes

    conv.v.meas = ccrun.fg_meta[0].range.start * conv.sim_vs_pars.gain;        // Set v_meas to voltage on load
    conv.i.meas.signal[REG_MEAS_UNFILTERED] = ccrun.fg_meta[0].range.start;    // i_meas
    conv.b.meas.signal[REG_MEAS_UNFILTERED] = ccrun.fg_meta[0].range.start;    // b_meas

    // Initialize load model for simulation using the sim_tc_error factor to mismatch the regulation

    regConvInitSimLoad(&conv, ccpars_global.reg_mode[0], ccpars_load.sim_tc_error);

    // Initialize voltage source model history according to the actuation because for CURRENT_REF,
    // the model is used for the current in the load rather than the voltage from the voltage source

    if(conv.actuation == REG_VOLTAGE_REF)
    {
        conv.v.ref_sat     =
        conv.v.ref_limited =
        conv.v.ref         = regSimVsInitHistory(&conv.sim_vs_pars, &conv.sim_vs_vars, conv.v.meas);
    }
    else // Actuation is CURRENT_REF
    {
        conv.ref_limited = ccrun.fg_meta[0].range.start;
        conv.sim_load_vars.magnet_current = conv.ref_limited * conv.sim_vs_pars.gain;
        regSimVsInitHistory(&conv.sim_vs_pars, &conv.sim_vs_vars, conv.sim_load_vars.magnet_current);
    }

    // Initialize measurement delay structures for simulated measurement of the field, current and voltage.
    // The -1.0 is because the simulated measurement applies to the following iteration, so 1 iteration
    // of delay is always present.

    regDelayInitDelay(&conv.b.sim.meas_delay, ccpars_vs.v_ref_delay_iters + ccpars_meas.b_delay_iters - 1.0);
    regDelayInitDelay(&conv.i.sim.meas_delay, ccpars_vs.v_ref_delay_iters + ccpars_meas.i_delay_iters - 1.0);
    regDelayInitDelay(&conv.v.sim.meas_delay, ccpars_vs.v_ref_delay_iters + ccpars_meas.v_delay_iters - 1.0);

    // Initialize simulated measurement delay histories

    regDelayInitVars(&conv.b.sim.meas_delay, conv.b.meas.signal[REG_MEAS_UNFILTERED]);
    regDelayInitVars(&conv.i.sim.meas_delay, conv.i.meas.signal[REG_MEAS_UNFILTERED]);
    regDelayInitVars(&conv.v.sim.meas_delay, conv.v.meas);

    // Initialize field measurement filter

    if(ccpars_meas.b_fir_lengths[0] == 0 || ccpars_meas.b_fir_lengths[1] == 0)
    {
        ccTestPrintError("B_FIR_LENGTHS must not be zero");
        return(EXIT_FAILURE);
    }

    free(conv.b.meas.fir_buf[0]);

    regMeasFilterInitBuffer(&conv.b.meas, calloc((ccpars_meas.b_fir_lengths[0] + ccpars_meas.b_fir_lengths[1] +
                                                 ccpars_breg.period_iters),sizeof(uint32_t)));

    regMeasFilterInit(&conv.b.meas, ccpars_meas.b_fir_lengths, ccpars_breg.period_iters,
                      ccpars_limits.b.pos, ccpars_limits.b.neg, ccpars_meas.b_delay_iters);

    regMeasSetRegSelect(&conv.b.meas, ccpars_meas.b_reg_select);

    // Initialize current measurement filter

    if(ccpars_meas.i_fir_lengths[0] == 0 || ccpars_meas.i_fir_lengths[1] == 0)
    {
        ccTestPrintError("I_FIR_LENGTHS must not be zero");
        return(EXIT_FAILURE);
    }

    free(conv.i.meas.fir_buf[0]);

    regMeasFilterInitBuffer(&conv.i.meas, calloc((ccpars_meas.i_fir_lengths[0] + ccpars_meas.i_fir_lengths[1] +
                                                 ccpars_ireg.period_iters),sizeof(uint32_t)));

    regMeasFilterInit(&conv.i.meas, ccpars_meas.i_fir_lengths, ccpars_ireg.period_iters,
                      ccpars_limits.i.pos, ccpars_limits.i.neg, ccpars_meas.i_delay_iters);

    regMeasSetRegSelect(&conv.i.meas, ccpars_meas.i_reg_select);

    // Initialize simulation of measurement noise and tone

    regMeasSetNoiseAndTone(&conv.b.sim.noise_and_tone, ccpars_meas.b_sim_noise_pp, ccpars_meas.b_sim_tone_amp,
                           ccpars_meas.tone_half_period_iters);

    regMeasSetNoiseAndTone(&conv.i.sim.noise_and_tone, ccpars_meas.i_sim_noise_pp, ccpars_meas.i_sim_tone_amp,
                           ccpars_meas.tone_half_period_iters);

    regMeasSetNoiseAndTone(&conv.v.sim.noise_and_tone, ccpars_meas.v_sim_noise_pp, 0.0, 0);

    regConvSimulateRT(&conv, 0.0);

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccInitRegulation(struct ccpars_reg_pars *reg_pars, struct reg_conv_signal *reg_signal, enum reg_mode mode, char *label)
{
    // Try to initialize RST

    if(regConvRstInit(&conv, reg_signal, mode, REG_OPERATIONAL_RST_PARS, reg_pars->period_iters,
                      reg_pars->auxpole1_hz, reg_pars->auxpoles2_hz, reg_pars->auxpoles2_z,
                      reg_pars->auxpole4_hz, reg_pars->auxpole5_hz,
                      reg_pars->pure_delay_periods,
                      reg_pars->track_delay_periods,
                      reg_pars->rst.r,
                      reg_pars->rst.s,
                      reg_pars->rst.t) == 0)
     {
        printf("Fatal - failed to initialize %s RST regulator: next RST parameters structure still pending switch by RT thread\n", label);
        exit(EXIT_FAILURE);
     }

    if(reg_signal->op_rst_pars.debug->status == REG_FAULT)
    {
        ccTestPrintError("failed to initialize %s RST regulator: S has unstable poles - try reducing AUXPOLE frequencies", label);
        return(EXIT_FAILURE);
    }

    if(reg_signal->op_rst_pars.debug->status == REG_WARNING)
    {
        ccTestPrintError("%s RST regulator warning: Modulus Margin (%.2f) is less than %.1f - try reducing AUXPOLE frequencies",
                label, reg_signal->op_rst_pars.debug->modulus_margin, REG_MM_WARNING_THRESHOLD);
    }

    reg_signal->err_rate = reg_pars->err_rate;

    return(EXIT_SUCCESS);
}
// EOF

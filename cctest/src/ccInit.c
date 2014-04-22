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

    // Set iteration period into libreg structure

    reg.iter_period = ccpars_global.iter_period;

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

        if(i >= num_reg_modes)
        {
            ccpars_global.reg_mode[i] = ccpars_global.reg_mode[num_reg_modes-1];
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
        // Initialise load model structure

        regLoadInit(&reg_pars.load_pars, ccpars_load.ohms_ser, ccpars_load.ohms_par,
                     ccpars_load.ohms_mag, ccpars_load.henrys, ccpars_load.gauss_per_amp);

        // Initialise load saturation model

        regLoadInitSat(&reg_pars.load_pars, ccpars_load.henrys_sat, ccpars_load.i_sat_start, ccpars_load.i_sat_end);

        // Field regulation requires an inductive load

        if(ccrun.breg_flag == 1 && reg.iter_period > (3.0 * reg_pars.load_pars.tc))
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
         // Initialise field and current measurement trip/low/zero limits

        regLimMeasInit(&reg.lim_b_meas,
                       ccpars_limits.b.pos, ccpars_limits.b.neg,
                       ccpars_limits.b.pos * LOW_MEAS_FACTOR,
                       ccpars_limits.b.pos * ZERO_MEAS_FACTOR,
                       ccpars_limits.invert_limits);

        regLimMeasInit(&reg.lim_i_meas,
                       ccpars_limits.i.pos, ccpars_limits.i.neg,
                       ccpars_limits.i.pos * LOW_MEAS_FACTOR,
                       ccpars_limits.i.pos * ZERO_MEAS_FACTOR,
                       ccpars_limits.invert_limits);

        // Initialise field, current and voltage reference pos/min/neg/rate limits

        regLimRefInit (&reg.lim_b_ref, ccpars_limits.b.pos, ccpars_limits.b.neg, ccpars_limits.b.rate,
                       ccpars_limits.invert_limits);

        regLimRefInit (&reg.lim_i_ref, ccpars_limits.i.pos, ccpars_limits.i.neg, ccpars_limits.i.rate,
                       ccpars_limits.invert_limits);

        regLimVrefInit(&reg.lim_v_ref, ccpars_limits.v.pos, ccpars_limits.v.neg, ccpars_limits.v.rate,
                       ccpars_limits.i_quadrants41, ccpars_limits.v_quadrants41, ccpars_limits.invert_limits);

        // Initialise field, current and voltage regulation error warning/fault thresholds

        regErrInitLimits(&reg.b_err, ccpars_limits.b_err_warning, ccpars_limits.b_err_fault);

        regErrInitLimits(&reg.i_err, ccpars_limits.i_err_warning, ccpars_limits.i_err_fault);

        regErrInitLimits(&reg.v_err, ccpars_limits.v_err_warning, ccpars_limits.v_err_fault);
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

    // Initialise v_ref limits in case ccrefCheckConverterLimits() is used

    regLimVrefInit(&ccrun.fg_lim_v_ref, ccpars_limits.v.pos, ccpars_limits.v.neg,
                   ccpars_limits.v.rate, ccpars_limits.i_quadrants41, ccpars_limits.v_quadrants41,
                   CC_DISABLED);

    // Initialise the reference functions

    for(func_idx = 0, exit_status = EXIT_SUCCESS ;
        func_idx < ccrun.num_functions && exit_status == EXIT_SUCCESS;
        func_idx++)
    {
        // Initialise pointer to function generation limits when FG_LIMITS is ENABLED

        if(ccpars_global.fg_limits == CC_ENABLED)
        {
            ccrun.fg_limits->user_data = ccpars_global.reg_mode[func_idx];

            switch(ccrun.fg_limits->user_data)
            {
                case REG_FIELD:   ccrun.fg_limits = &ccpars_limits.b;       break;
                case REG_CURRENT: ccrun.fg_limits = &ccpars_limits.i;       break;
                case REG_VOLTAGE: ccrun.fg_limits = &ccpars_limits.v;       break;
            }

            ccrun.fg_limits->user_check_limits = ccRefCheckConverterLimits;
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
uint32_t ccInitRegulation(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t status = 0;

    // Reset reg.mode since regSetMode & regSetVoltageMode are change sensitive

    reg.mode = REG_NONE;

    // Initialise FIELD regulator if field regulation used by at least one function

    if(ccrun.breg_flag == 1)
    {
        status = regRstInit(&reg_pars.b_rst_pars,
                            reg.iter_period, ccpars_breg.period_iters, &reg_pars.load_pars,
                            ccpars_breg.clbw, ccpars_breg.clbw2, ccpars_breg.z,
                            ccpars_breg.clbw3, ccpars_breg.clbw4,
                            regCalcPureDelay(&reg, &reg_pars),
                            ccpars_breg.track_delay_periods,
                            REG_FIELD, &ccpars_breg.rst);

        if(status != REG_OK)
        {
            ccTestPrintError("FIELD RST regulator failed to initialise: S[0] is less than 1.0E-10");
            return(EXIT_FAILURE);
        }
    }

    // Initialise CURRENT regulator if current regulation used by at least one function

    if(ccrun.ireg_flag == 1)
    {
        status = regRstInit(&reg_pars.i_rst_pars,
                            reg.iter_period, ccpars_ireg.period_iters, &reg_pars.load_pars,
                            ccpars_ireg.clbw, ccpars_ireg.clbw2, ccpars_ireg.z,
                            ccpars_ireg.clbw3, ccpars_ireg.clbw4,
                            regCalcPureDelay(&reg, &reg_pars),
                            ccpars_ireg.track_delay_periods,
                            REG_CURRENT, &ccpars_ireg.rst);
        if(status != REG_OK)
        {
            ccTestPrintError("FIELD RST regulator failed to initialise: S[0] is less than 1.0E-10");
            return(EXIT_FAILURE);
        }
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccInitSimulation(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    // Check that VS V_REF_DELAY_ITERS is at least 1

    if(ccpars_vs.v_ref_delay_iters < 1.0)
    {
        ccTestPrintError("VS V_REF_DELAY_ITERS (%g) must be >= 1.0",ccpars_vs.v_ref_delay_iters);
        return(EXIT_FAILURE);
    }

    // By default use voltage source simulation transfer function directly

    reg_pars.sim_vs_pars = ccpars_vs.sim_vs_pars;

    // If VS BANDWIDTH is greater than zero then initialise the voltage source simulation model coefficients
    // using Tustin algorithm.

    regSimVsInit(&reg_pars.sim_vs_pars, reg.iter_period, ccpars_vs.bandwidth,
                 ccpars_vs.z, ccpars_vs.tau_zero);

    // Initialise voltage source model gain and stop if gain error is more than 5%
    // This also calculates the vs_undersampled_flag for sim_load_pars

    reg_pars.sim_load_pars.vs_undersampled_flag = regSimVsInitGain(&reg_pars.sim_vs_pars, &reg.sim_vs_vars,
                                                                   ccpars_vs.v_ref_delay_iters);

    if(fabs(reg_pars.sim_vs_pars.gain - 1.0) > 0.05)
    {
        ccTestPrintError("voltage source model gain (%.3f) has an error of more than 5%%",
                         reg_pars.sim_vs_pars.gain);
        return(EXIT_FAILURE);
    }

    // First set the V/I/B measurements to cover all three regulation modes

    reg.v_meas            = ccrun.fg_meta[0].range.start * reg_pars.sim_vs_pars.gain; // Set v_meas to voltage on load
    reg.i_meas.unfiltered = ccrun.fg_meta[0].range.start;                             // i_meas
    reg.b_meas.unfiltered = ccrun.fg_meta[0].range.start;                             // b_meas

    // Initialise load model for simulation using the sim_tc_error factor to mismatch the regulation

    regSetSimLoad(&reg, &reg_pars, ccpars_global.reg_mode[0], ccpars_load.sim_tc_error);

    // Initialise voltage source model history to allow simulation to start with a non-zero voltage
    // This is not needed in a real converter controller because the voltage always starts at zero

    reg.v_ref_sat     =
    reg.v_ref_limited =
    reg.v_ref         = regSimVsInitHistory(&reg_pars.sim_vs_pars, &reg.sim_vs_vars, reg.v_meas);

    // Initialise measurement delay structures for simulated field, current and voltage in the
    // load and the measurements of these values in the load. The delay is adjusted by -1.0
    // iterations because the the simulation is used at the start of the next iteration, so
    // one iteration period has elapsed anyway. For this reason, v_ref_delay_iters must be
    // at least 1.

    regDelayInitDelays(&reg.b_sim.delay,
                       reg_pars.sim_load_pars.vs_undersampled_flag && reg_pars.sim_load_pars.load_undersampled_flag,
                       ccpars_vs.v_ref_delay_iters - 1.0,
                       ccpars_vs.v_ref_delay_iters + ccpars_meas.b_delay_iters - 1.0);

    regDelayInitDelays(&reg.i_sim.delay,
                       reg_pars.sim_load_pars.vs_undersampled_flag && reg_pars.sim_load_pars.load_undersampled_flag,
                       ccpars_vs.v_ref_delay_iters - 1.0,
                       ccpars_vs.v_ref_delay_iters + ccpars_meas.i_delay_iters - 1.0);

    regDelayInitDelays(&reg.v_sim.delay,
                       reg_pars.sim_load_pars.vs_undersampled_flag,
                       ccpars_vs.v_ref_delay_iters - 1.0,
                       ccpars_vs.v_ref_delay_iters + ccpars_meas.v_delay_iters - 1.0);

    // Initialise simulated measurement delay histories

    regDelayInitVars(&reg.b_sim.delay, reg.b_meas.unfiltered);
    regDelayInitVars(&reg.i_sim.delay, reg.i_meas.unfiltered);
    regDelayInitVars(&reg.v_sim.delay, reg.v_meas);

    // Initialise field measurement filter

    free(reg.b_meas.fir_buf[0]);

    regMeasFilterInitBuffer(&reg.b_meas, calloc((ccpars_meas.b_fir_lengths[0] + ccpars_meas.b_fir_lengths[1] +
                                                 ccpars_breg.period_iters),sizeof(uint32_t)));

    regMeasFilterInit(&reg.b_meas, ccpars_meas.b_fir_lengths, ccpars_breg.period_iters, ccpars_meas.b_delay_iters);

    regMeasFilterInitMax(&reg.b_meas, ccpars_limits.b.pos, ccpars_limits.b.neg);

    regMeasSetReg(&reg.b_meas, ccpars_meas.b_reg_select);

    // Initialise current measurement filter

    free(reg.i_meas.fir_buf[0]);

    regMeasFilterInitBuffer(&reg.i_meas, calloc((ccpars_meas.i_fir_lengths[0] + ccpars_meas.i_fir_lengths[1] +
                                                 ccpars_ireg.period_iters),sizeof(uint32_t)));

    regMeasFilterInit(&reg.b_meas, ccpars_meas.i_fir_lengths, ccpars_ireg.period_iters, ccpars_meas.i_delay_iters);

    regMeasFilterInitMax(&reg.b_meas, ccpars_limits.i.pos, ccpars_limits.i.neg);

    regMeasSetReg(&reg.i_meas, ccpars_meas.i_reg_select);

    // Initialise simulation of measurement noise and tone

    regMeasSetNoiseAndTone(&reg.b_sim.noise_and_tone, ccpars_meas.b_sim_noise_pp, ccpars_meas.b_sim_tone_amp,
                           ccpars_meas.tone_half_period_iters);

    regMeasSetNoiseAndTone(&reg.i_sim.noise_and_tone, ccpars_meas.i_sim_noise_pp, ccpars_meas.i_sim_tone_amp,
                           ccpars_meas.tone_half_period_iters);

    regMeasSetNoiseAndTone(&reg.v_sim.noise_and_tone, ccpars_meas.v_sim_noise_pp, 0.0, 0);

    // Run first simulation to initialise measurement variables

    regSimulate(&reg, &reg_pars, 0.0);

    return(EXIT_SUCCESS);
}
// EOF

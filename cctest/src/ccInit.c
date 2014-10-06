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
#include <stddef.h>
#include <math.h>

// Include cctest program header files

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRef.h"
#include "ccSigs.h"
#include "ccRun.h"

/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccInitFunctions(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called when the RUN command is executed to prepare for the new run.  It checks that
  parameters are valid and initializes the functions.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t         i;
    uint32_t         num_reg_modes;
    uint32_t         func_idx;
    uint32_t         exit_status;
    struct fgfunc   *func;
    struct cccmds   *cmd;

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

        if(ccpars_global.actuation == REG_CURRENT_REF)
        {
            if(ccpars_global.reg_mode[i] != REG_CURRENT)
            {
                ccTestPrintError("GLOBAL REG_MODE must CURRENT when GLOBAL ACTUATION is CURRENT");
                return(EXIT_FAILURE);
            }
        }

        switch(ccpars_global.reg_mode[i])
        {
            case REG_FIELD:   ccrun.breg_flag = 1;       break;
            case REG_CURRENT: ccrun.ireg_flag = 1;       break;
            default:                                     break;
        }
    }

    // if GLOBAL REVERSE_TIME is ENALBED

    if(ccpars_global.reverse_time == REG_ENABLED)
    {
        // SIM_LOAD must be DISABLED
        if(ccpars_global.sim_load == REG_ENABLED)
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

    if((ccrun.breg_flag == 1 || ccrun.ireg_flag == 1) && ccpars_global.sim_load != REG_ENABLED)
    {
        ccTestPrintError("GLOBAL SIM_LOAD must be ENABLED if REG_MODE is FIELD or CURRENT");
        return(EXIT_FAILURE);
    }

    // Check if GLOBAL ABORT_TIME is specified that FG_LIMITS is ENABLED

    if(ccpars_global.abort_time > 0.0 && ccpars_global.fg_limits == REG_DISABLED)
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

    // Reset function generation limits pointer by default

    ccrun.fg_limits = NULL;

    // Initialize the reference functions

    for(func_idx = 0, exit_status = EXIT_SUCCESS ;
        func_idx < ccrun.num_functions && exit_status == EXIT_SUCCESS;
        func_idx++)
    {
        // Initialize pointer to function generation limits when FG_LIMITS is ENABLED

        if(ccpars_global.fg_limits == REG_ENABLED)
        {
            switch(ccpars_global.reg_mode[func_idx])
            {
                case REG_NONE:                                              break;
                case REG_FIELD:   ccrun.fg_limits = &ccpars_limits.b;       break;
                case REG_CURRENT: ccrun.fg_limits = &ccpars_limits.i;       break;
                case REG_VOLTAGE: ccrun.fg_limits = &ccpars_limits.v;       break;
            }
        }

        // Try to arm the next function

        func = &funcs[ccpars_global.function[func_idx]];

        exit_status = func->init_func(&ccrun.fg_meta[func_idx]);

        // Mark command for this function as enabled to include parameters in FLOT colorbox pop-up

        cmds[func->cmd_idx].enabled = 1;
    }

    // Enable the commands whose parameters should be included in FLOT colorbox pop-up

    cmds[CMD_GLOBAL].enabled = 1;
    cmds[CMD_LIMITS].enabled = 1;

    // Initialize converter structure

    regConvInit(&conv, ccpars_global.iter_period_us, ccrun.breg_flag, ccrun.ireg_flag);

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccInitSimLoad(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function is called when the RUN command is executed to prepare for the new run.  It checks that
  parameters are valid and initialises the simulation.
\*---------------------------------------------------------------------------------------------------------*/
{
    static struct reg_meas_signal invalid_meas = { 0.0, REG_MEAS_SIGNAL_INVALID };

    // Enabled of logging of simulation related parameters in FLOT colorbox pop-up

    cmds[CMD_GLOBAL].enabled = 1;
    cmds[CMD_LOAD  ].enabled = 1;
    cmds[CMD_MEAS  ].enabled = 1;
    cmds[CMD_VS    ].enabled = 1;
    cmds[CMD_BREG  ].enabled = ccrun.breg_flag;
    cmds[CMD_IREG  ].enabled = ccrun.ireg_flag;

    // Prepare an invalid signal to allow recovery from invalid signals to be tested

    regConvInitMeas(&conv, &invalid_meas, &invalid_meas, &invalid_meas);

    // Check measurement filter lengths

    if(ccpars_meas.b_fir_lengths[0] == 0 || ccpars_meas.b_fir_lengths[1] == 0)
    {
        ccTestPrintError("B_FIR_LENGTHS must not be zero");
        return(EXIT_FAILURE);
    }

    if(ccpars_meas.i_fir_lengths[0] == 0 || ccpars_meas.i_fir_lengths[1] == 0)
    {
        ccTestPrintError("I_FIR_LENGTHS must not be zero");
        return(EXIT_FAILURE);
    }

    // Prepare measurement FIR buffers

    free(conv.b.meas.fir_buf[0]);

    regMeasFilterInitBuffer(&conv.b.meas, calloc((ccpars_meas.b_fir_lengths[0] + ccpars_meas.b_fir_lengths[1] +
                                                  ccpars_breg.period_iters),sizeof(uint32_t)));

    // Initialize current measurement filter

    free(conv.i.meas.fir_buf[0]);

    regMeasFilterInitBuffer(&conv.i.meas, calloc((ccpars_meas.i_fir_lengths[0] + ccpars_meas.i_fir_lengths[1] +
                                                  ccpars_ireg.period_iters),sizeof(uint32_t)));

    // Initialize libreg parameter pointers to cctest variables

    regConvParInitPointer(&conv,global_actuation              ,&ccpars_global.actuation);
    regConvParInitPointer(&conv,global_reg_err_rate           ,&ccpars_global.reg_err_rate);

    regConvParInitPointer(&conv,breg_period_iters             ,&ccpars_breg.period_iters);
    regConvParInitPointer(&conv,breg_op_pure_delay_periods    ,&ccpars_breg.pure_delay_periods);
    regConvParInitPointer(&conv,breg_op_track_delay_periods   ,&ccpars_breg.track_delay_periods);
    regConvParInitPointer(&conv,breg_op_auxpole1_hz           ,&ccpars_breg.auxpole1_hz);
    regConvParInitPointer(&conv,breg_op_auxpoles2_hz          ,&ccpars_breg.auxpoles2_hz);
    regConvParInitPointer(&conv,breg_op_auxpoles2_z           ,&ccpars_breg.auxpoles2_z);
    regConvParInitPointer(&conv,breg_op_auxpole4_hz           ,&ccpars_breg.auxpole4_hz);
    regConvParInitPointer(&conv,breg_op_auxpole5_hz           ,&ccpars_breg.auxpole5_hz);
    regConvParInitPointer(&conv,breg_op_r                     ,&ccpars_breg.rst.r);
    regConvParInitPointer(&conv,breg_op_s                     ,&ccpars_breg.rst.s);
    regConvParInitPointer(&conv,breg_op_t                     ,&ccpars_breg.rst.t);

    regConvParInitPointer(&conv,ireg_period_iters             ,&ccpars_ireg.period_iters);
    regConvParInitPointer(&conv,ireg_op_pure_delay_periods    ,&ccpars_ireg.pure_delay_periods);
    regConvParInitPointer(&conv,ireg_op_track_delay_periods   ,&ccpars_ireg.track_delay_periods);
    regConvParInitPointer(&conv,ireg_op_auxpole1_hz           ,&ccpars_ireg.auxpole1_hz);
    regConvParInitPointer(&conv,ireg_op_auxpoles2_hz          ,&ccpars_ireg.auxpoles2_hz);
    regConvParInitPointer(&conv,ireg_op_auxpoles2_z           ,&ccpars_ireg.auxpoles2_z);
    regConvParInitPointer(&conv,ireg_op_auxpole4_hz           ,&ccpars_ireg.auxpole4_hz);
    regConvParInitPointer(&conv,ireg_op_auxpole5_hz           ,&ccpars_ireg.auxpole5_hz);
    regConvParInitPointer(&conv,ireg_op_r                     ,&ccpars_ireg.rst.r);
    regConvParInitPointer(&conv,ireg_op_s                     ,&ccpars_ireg.rst.s);
    regConvParInitPointer(&conv,ireg_op_t                     ,&ccpars_ireg.rst.t);

    regConvParInitPointer(&conv,limits_b_pos                  ,&ccpars_limits.b.pos);
    regConvParInitPointer(&conv,limits_b_min                  ,&ccpars_limits.b.min);
    regConvParInitPointer(&conv,limits_b_neg                  ,&ccpars_limits.b.neg);
    regConvParInitPointer(&conv,limits_b_rate                 ,&ccpars_limits.b.rate);
    regConvParInitPointer(&conv,limits_b_acceleration         ,&ccpars_limits.b.acceleration);
    regConvParInitPointer(&conv,limits_b_closeloop            ,&ccpars_limits.b_closeloop);
    regConvParInitValue  (&conv,limits_b_zero_div_pos, 0      ,0.01);
    regConvParInitPointer(&conv,limits_b_err_warning          ,&ccpars_limits.b_err_warning);
    regConvParInitPointer(&conv,limits_b_err_fault            ,&ccpars_limits.b_err_fault);

    regConvParInitPointer(&conv,limits_i_pos                  ,&ccpars_limits.i.pos);
    regConvParInitPointer(&conv,limits_i_min                  ,&ccpars_limits.i.min);
    regConvParInitPointer(&conv,limits_i_neg                  ,&ccpars_limits.i.neg);
    regConvParInitPointer(&conv,limits_i_rate                 ,&ccpars_limits.i.rate);
    regConvParInitPointer(&conv,limits_i_acceleration         ,&ccpars_limits.i.acceleration);
    regConvParInitPointer(&conv,limits_i_closeloop            ,&ccpars_limits.i_closeloop);
    regConvParInitValue  (&conv,limits_i_zero_div_pos, 0      ,0.01);
    regConvParInitPointer(&conv,limits_i_err_warning          ,&ccpars_limits.i_err_warning);
    regConvParInitPointer(&conv,limits_i_err_fault            ,&ccpars_limits.i_err_fault);

    regConvParInitPointer(&conv,limits_i_rms_tc               ,&ccpars_limits.i_rms_tc);
    regConvParInitPointer(&conv,limits_i_rms_warning          ,&ccpars_limits.i_rms_warning);
    regConvParInitPointer(&conv,limits_i_rms_fault            ,&ccpars_limits.i_rms_fault);
    regConvParInitPointer(&conv,limits_i_rms_load_tc          ,&ccpars_limits.i_rms_load_tc);
    regConvParInitPointer(&conv,limits_i_rms_load_warning     ,&ccpars_limits.i_rms_load_warning);
    regConvParInitPointer(&conv,limits_i_rms_load_fault       ,&ccpars_limits.i_rms_load_fault);

    regConvParInitPointer(&conv,limits_i_quadrants41          ,&ccpars_limits.i_quadrants41);
    regConvParInitPointer(&conv,limits_v_pos                  ,&ccpars_limits.v.pos);
    regConvParInitPointer(&conv,limits_v_neg                  ,&ccpars_limits.v.neg);
    regConvParInitPointer(&conv,limits_v_rate                 ,&ccpars_limits.v.rate);
    regConvParInitPointer(&conv,limits_v_acceleration         ,&ccpars_limits.v.acceleration);
    regConvParInitPointer(&conv,limits_v_err_warning          ,&ccpars_limits.v_err_warning);
    regConvParInitPointer(&conv,limits_v_err_fault            ,&ccpars_limits.v_err_fault);
    regConvParInitPointer(&conv,limits_v_quadrants41          ,&ccpars_limits.v_quadrants41);
    regConvParInitPointer(&conv,limits_invert                 ,&ccpars_limits.invert);

    regConvParInitPointer(&conv,load_ohms_ser                 ,&ccpars_load.ohms_ser);
    regConvParInitPointer(&conv,load_ohms_par                 ,&ccpars_load.ohms_par);
    regConvParInitPointer(&conv,load_ohms_mag                 ,&ccpars_load.ohms_mag);
    regConvParInitPointer(&conv,load_henrys                   ,&ccpars_load.henrys);
    regConvParInitPointer(&conv,load_henrys_sat               ,&ccpars_load.henrys_sat);
    regConvParInitPointer(&conv,load_i_sat_start              ,&ccpars_load.i_sat_start);
    regConvParInitPointer(&conv,load_i_sat_end                ,&ccpars_load.i_sat_end);
    regConvParInitPointer(&conv,load_gauss_per_amp            ,&ccpars_load.gauss_per_amp);
    regConvParInitPointer(&conv,load_sim_tc_error             ,&ccpars_load.sim_tc_error);

    regConvParInitPointer(&conv,meas_b_reg_select             ,&ccpars_meas.b_reg_select);
    regConvParInitPointer(&conv,meas_i_reg_select             ,&ccpars_meas.i_reg_select);
    regConvParInitPointer(&conv,meas_b_delay_iters            ,&ccpars_meas.b_delay_iters);
    regConvParInitPointer(&conv,meas_i_delay_iters            ,&ccpars_meas.i_delay_iters);
    regConvParInitPointer(&conv,meas_v_delay_iters            ,&ccpars_meas.v_delay_iters);
    regConvParInitPointer(&conv,meas_b_fir_lengths            ,&ccpars_meas.b_fir_lengths);
    regConvParInitPointer(&conv,meas_i_fir_lengths            ,&ccpars_meas.i_fir_lengths);
    regConvParInitPointer(&conv,meas_b_sim_noise_pp           ,&ccpars_meas.b_sim_noise_pp);
    regConvParInitPointer(&conv,meas_i_sim_noise_pp           ,&ccpars_meas.i_sim_noise_pp);
    regConvParInitPointer(&conv,meas_v_sim_noise_pp           ,&ccpars_meas.v_sim_noise_pp);
    regConvParInitPointer(&conv,meas_tone_half_period_iters   ,&ccpars_meas.tone_half_period_iters);
    regConvParInitPointer(&conv,meas_b_sim_tone_amp           ,&ccpars_meas.b_sim_tone_amp);
    regConvParInitPointer(&conv,meas_i_sim_tone_amp           ,&ccpars_meas.i_sim_tone_amp);

    regConvParInitPointer(&conv,vs_v_ref_delay_iters          ,&ccpars_vs.v_ref_delay_iters);
    regConvParInitPointer(&conv,vs_bandwidth                  ,&ccpars_vs.bandwidth);
    regConvParInitPointer(&conv,vs_z                          ,&ccpars_vs.z);
    regConvParInitPointer(&conv,vs_tau_zero                   ,&ccpars_vs.tau_zero);
    regConvParInitPointer(&conv,vs_sim_num                    ,&ccpars_vs.sim_vs_pars.num);
    regConvParInitPointer(&conv,vs_sim_den                    ,&ccpars_vs.sim_vs_pars.den);

    // Initialize simulation

    regConvInitSim(&conv, ccpars_global.reg_mode[0], ccrun.fg_meta[0].range.start);


    // Check simulated voltage source gain

    if(fabs(conv.sim_vs_pars.gain - 1.0) > 0.05)
    {
        ccTestPrintError("voltage source model gain (%.3f) has an error of more than 5%%", conv.sim_vs_pars.gain);
        return(EXIT_FAILURE);
    }

    // Check initialization of field regulation

    if(ccrun.breg_flag == 1)
    {
        if(conv.iter_period > (3.0 * conv.load_pars.tc))
        {
            ccTestPrintError("REG_MODE of FIELD is not permitted for a resistive circuit "
                             "(circuit time constant is less than 1/3 x iteration period)");
            return(EXIT_FAILURE);
        }

        if(conv.b.op_rst_pars.debug->status == REG_FAULT)
        {
            ccTestPrintError("failed to initialize FIELD RST regulator: S has unstable poles - try reducing AUXPOLE frequencies");
            return(EXIT_FAILURE);
        }

        if(conv.b.op_rst_pars.debug->status == REG_WARNING)
        {
            ccTestPrintError("FIELD RST regulator warning: Modulus Margin (%.2f) is less than %.1f - try reducing AUXPOLE frequencies",
                             conv.b.op_rst_pars.debug->modulus_margin, REG_MM_WARNING_THRESHOLD);
        }
    }

    // Check initialization of current regulation

    if(ccrun.ireg_flag == 1)
    {
        if(conv.i.op_rst_pars.debug->status == REG_FAULT)
        {
            ccTestPrintError("failed to initialize CURRENT RST regulator: S has unstable poles - try reducing AUXPOLE frequencies");
            return(EXIT_FAILURE);
        }

        if(conv.i.op_rst_pars.debug->status == REG_WARNING)
        {
            ccTestPrintError("CURRENT RST regulator warning: Modulus Margin (%.2f) is less than %.1f - try reducing AUXPOLE frequencies",
                             conv.i.op_rst_pars.debug->modulus_margin, REG_MM_WARNING_THRESHOLD);
        }
    }

    return(EXIT_SUCCESS);
}
// EOF

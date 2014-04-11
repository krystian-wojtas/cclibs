/*---------------------------------------------------------------------------------------------------------*\
  File:     ccTest.c                                                                    Copyright CERN 2014

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

  Purpose:  Test program for libfg and libreg (function generation and regulation)

  Authors:  Quentin.King@cern.ch
            Stephen.Page@cern.ch

\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Declare all variables in ccTest.c

#define GLOBALS

// Include cctest program header files

#include "ccPars.h"
#include "ccRef.h"
#include "ccSigs.h"
#include "ccRun.h"

/*---------------------------------------------------------------------------------------------------------*/
int main(int argc, char **argv)
/*---------------------------------------------------------------------------------------------------------*/
{
    int status = 0;

    printf(">%s<\n",argv[0]);

    // If no arguments supplied, read from stdin

    if(argc == 1)
    {
        status = ccParsParseLine("read stdin");
    }
    else
    {
        // else process all arguments unless an error is reported

        while(status == 0 && --argc > 0)
        {
            status = ccParsParseLine(*(++argv));
        }
    }

    // Report status as exit code

    exit(status);
}
/*---------------------------------------------------------------------------------------------------------*/
static void PrepareLoad(void)
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

        if(ccpars_global.reg_mode == REG_FIELD && reg.iter_period > (3.0 * reg_pars.load_pars.tc))
        {
            fputs("Error : GAUSS units not permitted for a resistive circuit "
                  "(circuit time constant is less than 1/3 x iteration period)\n",stderr);
            exit(EXIT_FAILURE);
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void PrepareLimits(void)
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
}
/*---------------------------------------------------------------------------------------------------------*/
static void PrepareFunction(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    // If FG_LIMITS is ENABLED

    if(ccpars_global.fg_limits == CC_ENABLED)
    {
        // Set fg_limits pointer according to UNITS

        switch(ccpars_global.reg_mode)
        {
        case REG_FIELD:   ccpars_limits.fg = &ccpars_limits.b;       break;
        case REG_CURRENT: ccpars_limits.fg = &ccpars_limits.i;       break;
        case REG_VOLTAGE: ccpars_limits.fg = &ccpars_limits.v;       break;
        }

        // If AMPS or GAUSS will be regulated then the limits checking includes the converter model

        if(ccpars_global.reg_mode != REG_VOLTAGE)
        {
            ccpars_limits.fg->user_check_limits = ccrefCheckConverterLimits;

            // Initialise v_ref limits for ccrefCheckConverterLimits() which is called when
            // arming a function to check the voltage available is sufficient

            regLimVrefInit(&ccpars_limits.fg_v_ref, ccpars_limits.v.pos, ccpars_limits.v.neg,
                           ccpars_limits.v.rate, ccpars_limits.i_quadrants41, ccpars_limits.v_quadrants41, 
                           CC_DISABLED);
        }
    }

    // Initialise the reference function

    func[ccpars_global.function].init_func();
}
/*---------------------------------------------------------------------------------------------------------*/
static void PrepareSimulation(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    if(ccpars_global.sim_load == CC_ENABLED)
    {
        // By default use voltage source simulation transfer function directly

        reg_pars.sim_vs_pars = ccpars_vs.sim_vs_pars;

        // If VS bandwidth defined the try to initialise voltage source simulation model coefficients
        // using Tustin algorithm.

        if(ccpars_vs.bandwidth > 0.0)
        {
            regSimVsInit(&reg_pars.sim_vs_pars, reg.iter_period, ccpars_vs.bandwidth,
                         ccpars_vs.z, ccpars_vs.tau_zero);
        }

        // Initialise voltage source model gain and stop if gain error is more than 5%
        // This also calculates the vs_undersampled_flag for sim_load_pars

        reg_pars.sim_load_pars.vs_undersampled_flag = regSimVsInitGain(&reg_pars.sim_vs_pars, &reg.sim_vs_vars,
                                                                       ccpars_vs.v_ref_delay_iters);

        if(fabs(reg_pars.sim_vs_pars.gain - 1.0) > 0.05)
        {
            fprintf(stderr,"Error : Voltage source model gain (%.3f) has an error of more than 5%%\n",reg_pars.sim_vs_pars.gain);
            exit(EXIT_FAILURE);
        }

        // First set the V/I/B measurements to cover all three regulation modes

        reg.v_meas            = fg_meta.range.start * reg_pars.sim_vs_pars.gain;     // Set v_meas to voltage on load
        reg.i_meas.unfiltered = fg_meta.range.start;                                 // i_meas
        reg.b_meas.unfiltered = fg_meta.range.start;                                 // b_meas

        // Initialise load model for simulation using the sim_tc_error factor to mismatch the regulation

        regSetSimLoad(&reg, &reg_pars, ccpars_global.reg_mode, ccpars_load.sim_tc_error);

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
                                                     ccpars_reg_b.period_iters),sizeof(uint32_t)));

        regMeasFilterInit(&reg.b_meas, ccpars_meas.b_fir_lengths, ccpars_reg_b.period_iters, ccpars_meas.b_delay_iters);

        regMeasFilterInitMax(&reg.b_meas, ccpars_limits.b.pos, ccpars_limits.b.neg);

        regMeasSetReg(&reg.b_meas, ccpars_meas.b_reg_select);

        // Initialise current measurement filter

        free(reg.i_meas.fir_buf[0]);

        regMeasFilterInitBuffer(&reg.i_meas, calloc((ccpars_meas.i_fir_lengths[0] + ccpars_meas.i_fir_lengths[1] +
                                                     ccpars_reg_i.period_iters),sizeof(uint32_t)));

        regMeasFilterInit(&reg.b_meas, ccpars_meas.i_fir_lengths, ccpars_reg_i.period_iters, ccpars_meas.i_delay_iters);

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
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void PrepareRegulation(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t status = 0;

    // If voltage source will be simulated

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        reg.mode = REG_NONE;                    // Reset reg.mode since regSetMode & regSetVoltageMode are
                                                // change sensitive
        // If voltage regulation enabled

        if(ccpars_global.reg_mode == REG_VOLTAGE)
        {
            regSetVoltageMode(&reg, &reg_pars);
        }
        else
        {
            // Initialise limited reference to equal the starting value of the reference function

            reg.ref_rst = reg.ref_limited = fg_meta.range.start;

            // Calculate track delay in iteration periods

            switch(ccpars_global.reg_mode)
            {
            case REG_FIELD:  // Initialise field regulation

                status = regRstInit(&reg_pars.b_rst_pars,
                                    reg.iter_period, ccpars_reg_b.period_iters, &reg_pars.load_pars,
                                    ccpars_reg_b.clbw, ccpars_reg_b.clbw2, ccpars_reg_b.z,
                                    ccpars_reg_b.clbw3, ccpars_reg_b.clbw4,
                                    regCalcPureDelay(&reg, &reg_pars),
                                    ccpars_reg_b.track_delay_periods,
                                    REG_FIELD, &ccpars_reg_b.rst);


                // regSetMode requires reg.v_ref_limited to be set to the voltage reference that
                // corresponds to steady state reg.b_meas (last parameter is the rate of change)

                regSetMode(&reg, &reg_pars, REG_FIELD, reg.b_meas.unfiltered, 0.0);
                break;

            case REG_CURRENT:   // Initialise current regulation

                status = regRstInit(&reg_pars.i_rst_pars,
                                    reg.iter_period, ccpars_reg_i.period_iters, &reg_pars.load_pars,
                                    ccpars_reg_i.clbw, ccpars_reg_i.clbw2, ccpars_reg_i.z,
                                    ccpars_reg_i.clbw3, ccpars_reg_i.clbw4,
                                    regCalcPureDelay(&reg, &reg_pars),
                                    ccpars_reg_i.track_delay_periods,
                                    REG_CURRENT, &ccpars_reg_i.rst);

                // regSetMode requires reg.v_ref_limited to be set to the voltage reference that
                // corresponds to steady state reg.i_meas (last parameter is the rate of change)

                regSetMode(&reg, &reg_pars, REG_CURRENT, reg.i_meas.unfiltered, 0.0);
                break;
            }

            // Check for regulation initialisation failure - there is only one possible reason: S[0] is too small

            if(status != REG_OK)
            {
                fputs("Error : RST regulator failed to initialise: S[0] is less than 1.0E-10\n",stderr);
                exit(EXIT_FAILURE);
            }

            // Estimate the ref advance time

            regCalcRefAdvance(&reg, &reg_pars);
        }
    }
}
// EOF

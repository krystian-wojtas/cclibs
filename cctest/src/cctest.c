/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest.c                                                                    Copyright CERN 2011

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

// Declare all variables in cctest.c

#define GLOBALS

// Include libfg and libreg header files

#include "libfg/plep.h"
#include "libfg/pppl.h"
#include "libfg/spline.h"
#include "libfg/table.h"
#include "libfg/test.h"
#include "libfg/trim.h"
#include "libreg.h"

// Include cctest parameter header files

#include "pars/global.h"
#include "pars/limits.h"
#include "pars/load.h"
#include "pars/reg.h"
#include "pars/vs.h"

// Include cctest function data header files

#include "func/table.h"
#include "func/plep.h"
#include "func/pppl.h"
#include "func/test.h"
#include "func/trim.h"

// Include cctest program header files

#include "ccref.h"
#include "ccsigs.h"
#include "ccrun.h"

/*---------------------------------------------------------------------------------------------------------*/
static void PrepareLoad(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    // If load parameters supplied

    if(ccpars_load.status == 1)
    {
        // Initialise load model structure

        regLoadInit(&reg_pars.load_pars, ccpars_load.ohms_ser, ccpars_load.ohms_par,
                     ccpars_load.ohms_mag, ccpars_load.henrys, ccpars_load.gauss_per_amp);

        // Initialise load saturation model

        regLoadInitSat(&reg_pars.load_pars, ccpars_load.henrys_sat, ccpars_load.i_sat_start, ccpars_load.i_sat_end);

        // Field regulation requires an inductive load

        if(ccpars_global.units == REG_FIELD && reg.iter_period > (3.0 * reg_pars.load_pars.tc))
        {
            fputs("Error : GAUSS units not permitted for a resistive circuit "
                  "(circuit time constant is less than 1/3 x iteration period)\n",stderr);
            exit(EXIT_FAILURE);
        }

        // Initialise measurement filters

        regMeasFilterInit(&reg_pars.i_meas, &reg.i_meas, ccpars_load.i_meas_pars.num, ccpars_load.i_meas_pars.den);
        regMeasFilterInit(&reg_pars.b_meas, &reg.b_meas, ccpars_load.b_meas_pars.num, ccpars_load.b_meas_pars.den);
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
                       ccpars_limits.b.pos * ZERO_MEAS_FACTOR);

        regLimMeasInit(&reg.lim_i_meas,
                       ccpars_limits.i.pos, ccpars_limits.i.neg,
                       ccpars_limits.i.pos * LOW_MEAS_FACTOR,
                       ccpars_limits.i.pos * ZERO_MEAS_FACTOR);

        // Initialise field, current and voltage reference pos/min/neg/rate limits

        regLimRefInit (&reg.lim_b_ref, ccpars_limits.b.pos, ccpars_limits.b.neg, ccpars_limits.b.rate);

        regLimRefInit (&reg.lim_i_ref, ccpars_limits.i.pos, ccpars_limits.i.neg, ccpars_limits.i.rate);

        regLimVrefInit(&reg.lim_v_ref, ccpars_limits.v.pos, ccpars_limits.v.neg, ccpars_limits.v.rate,
                       ccpars_limits.i_quadrants41, ccpars_limits.v_quadrants41);

        // Initialise field, current and voltage regulation error warning/fault limits

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

        switch(ccpars_global.units)
        {
        case REG_FIELD:   ccpars_limits.fg = &ccpars_limits.b;       break;
        case REG_CURRENT: ccpars_limits.fg = &ccpars_limits.i;       break;
        case REG_VOLTAGE: ccpars_limits.fg = &ccpars_limits.v;       break;
        }

        // If AMPS or GAUSS will be regulated then the limits checking includes the converter model

        if(ccpars_global.units != REG_VOLTAGE)
        {
            ccpars_limits.fg->user_check_limits = ccrefCheckConverterLimits;

            regLimVrefInit(&ccpars_limits.fg_v_ref, ccpars_limits.v.pos, ccpars_limits.v.neg,
                           ccpars_limits.v.rate, ccpars_limits.i_quadrants41, ccpars_limits.v_quadrants41);
        }
    }

    // Initialise the reference function

    func[ccpars_global.function].init_func();
}
/*---------------------------------------------------------------------------------------------------------*/
static void PrepareSimulation(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    float    delay_in_iterations;

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        // By default use z-transform directly

        reg_pars.sim_vs_pars = ccpars_vs.sim_vs_pars;

        // If natural frequency defined the try to initialise voltage source simulation model coefficients
        // using Tustin algorithm.  if nat_freq > nyquist then z-transform is not changed

        if(ccpars_vs.natural_freq > 0.0)
        {
            regSimVsInit(&reg_pars.sim_vs_pars, reg.iter_period, ccpars_vs.natural_freq,
                         ccpars_vs.z, ccpars_vs.tau_zero);
        }

        // Initialise voltage source model gain and stop if gain error is more than 5%
        // This also calculates the vs_undersampled_flag in the sim_load_pars

        if(fabs(regSimVsInitGain(&reg_pars.sim_vs_pars, &reg_pars.sim_load_pars) - 1.0) > 0.05)
        {
            fprintf(stderr,"Error : Voltage source model gain (%.3f) has an error of more than 5%%\n",reg_pars.sim_vs_pars.gain);
            exit(EXIT_FAILURE);
        }

        // Initialise load model for simulation using the sim_tc_error factor to mismatch the regulation
        // First set the V/I/B measurements to cover all three regulation modes

        reg.v_meas.unfiltered = fg_meta.range.start * reg_pars.sim_vs_pars.gain;     // Set v_meas to voltage on load
        reg.i_meas.unfiltered = fg_meta.range.start;                                 // i_meas
        reg.b_meas.unfiltered = fg_meta.range.start;                                 // b_meas

        regSetSimLoad(&reg, &reg_pars, ccpars_global.units, ccpars_load.sim_tc_error);

        regSetMeasNoise(&reg, ccpars_vs.v_meas_noise, ccpars_load.b_meas_noise, ccpars_load.i_meas_noise);

        // Initialise voltage source model history to allow simulation to start with a non-zero voltage
        // This is not needed in a real converter controller because the voltage always starts at zero

        reg.v_ref_sat     =
        reg.v_ref_limited =
        reg.v_ref         = regSimVsInitHistory(&reg_pars.sim_vs_pars, &reg.sim_vs_vars, reg.v_meas.unfiltered);

        // Initialise measurement delay structures for simulated field, current and voltage measurements.
        // The delay must include the voltage reference delay and the measurement delay minus 1 period.
        // The -1 is because the measurement delays are run by calling regSetMeas() at the start of an
        // iteration using the voltage source and load simulation results calculated at the end of the
        // previous iteration.  So a delay of 1 iteration period is always present.  This means that the
        // voltage reference delay must be a minimum of one iteration period.

        delay_in_iterations = (ccpars_vs.v_ref_delay + ccpars_vs.v_meas_delay) / reg.iter_period  - 1.0;

        regDelayInitPars(&reg.v_meas_delay,
                         calloc(1+(size_t)delay_in_iterations, sizeof(float)),
                         delay_in_iterations,
                         reg_pars.sim_load_pars.vs_undersampled_flag);

        delay_in_iterations = (ccpars_vs.v_ref_delay + ccpars_load.i_meas_delay) / reg.iter_period - 1.0;

        regDelayInitPars(&reg.i_meas_delay,
                         calloc(1+(size_t)delay_in_iterations, sizeof(float)),
                         delay_in_iterations,
                         reg_pars.sim_load_pars.vs_undersampled_flag && reg_pars.sim_load_pars.load_undersampled_flag);

        delay_in_iterations = (ccpars_vs.v_ref_delay + ccpars_load.b_meas_delay)/ reg.iter_period - 1.0;

        regDelayInitPars(&reg.b_meas_delay,
                         calloc(1+(size_t)delay_in_iterations,
                         sizeof(float)),
                         delay_in_iterations,
                         reg_pars.sim_load_pars.vs_undersampled_flag && reg_pars.sim_load_pars.load_undersampled_flag);

        // Initialise voltage measurement filter

        regMeasFilterInit(&reg_pars.v_meas, &reg.v_meas, ccpars_vs.v_meas_pars.num, ccpars_vs.v_meas_pars.den);

        // Initialise measurement filter histories

        regMeasFilterInitHistory(&reg.v_meas, reg.v_meas.unfiltered);
        regMeasFilterInitHistory(&reg.i_meas, reg.i_meas.unfiltered);
        regMeasFilterInitHistory(&reg.b_meas, reg.b_meas.unfiltered);

        // Initialise measurement delay histories

        regDelayInitVars(&reg.v_meas_delay, reg.v_meas.unfiltered);
        regDelayInitVars(&reg.i_meas_delay, reg.i_meas.unfiltered);
        regDelayInitVars(&reg.b_meas_delay, reg.b_meas.unfiltered);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void PrepareRegulation(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t status = 0;
    float    delay_in_iterations;

    // If voltage source will be simulated

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        // Initialise voltage source error calculation variables

        if(ccpars_vs.track_delay > reg.iter_period)
        {
            delay_in_iterations = ccpars_vs.track_delay / reg.iter_period;
        }
        else
        {
            delay_in_iterations = 1.0;
        }

        regErrInitDelay(&reg.v_err, calloc(1+(size_t)delay_in_iterations, sizeof(float)),
                         ccpars_vs.track_delay, reg.iter_period);

        // If voltage regulation enabled

        reg.mode = REG_NONE;                    // Reset reg.mode since regSetMode & regSetVoltageMode are
                                                // change sensitive

        if(ccpars_global.units == REG_VOLTAGE)
        {
            regSetVoltageMode(&reg, &reg_pars);
        }
        else
        {
            // Initialise limited reference to equal the starting value of the reference function

            reg.ref_rst = reg.ref_limited = fg_meta.range.start;

            // Calculate track delay in iteration periods

            switch(ccpars_global.units)
            {
            case REG_FIELD:  // Initialise field regulation

                status = regRstInit(&reg_pars.b_rst_pars,
                                    reg.iter_period, ccpars_reg.period_iters, &reg_pars.load_pars,
                                    ccpars_reg.clbw, ccpars_reg.clbw2, ccpars_reg.z,
                                    ccpars_reg.clbw3, ccpars_reg.clbw4, ccpars_reg.pure_delay,
                                    REG_FIELD, ccpars_reg.decimate, &ccpars_reg.rst);

                delay_in_iterations = reg_pars.b_rst_pars.rst.track_delay / reg.iter_period;

                regErrInitDelay(&reg.b_err, calloc(1+(size_t)delay_in_iterations, sizeof(float)),
                                 reg_pars.b_rst_pars.rst.track_delay, reg.iter_period);

                // regSetMode requires reg.v_ref_limited to be set to the voltage reference that
                // corresponds to steady state reg.b_meas (last parameter is the rate of change)

                regSetMode(&reg, &reg_pars, REG_FIELD, reg.b_meas.unfiltered, 0.0);
                break;

            case REG_CURRENT:   // Initialise current regulation

                status = regRstInit(&reg_pars.i_rst_pars,
                                    reg.iter_period, ccpars_reg.period_iters, &reg_pars.load_pars,
                                    ccpars_reg.clbw, ccpars_reg.clbw2, ccpars_reg.z,
                                    ccpars_reg.clbw3, ccpars_reg.clbw4, ccpars_reg.pure_delay,
                                    REG_CURRENT, ccpars_reg.decimate, &ccpars_reg.rst);

                delay_in_iterations = reg_pars.i_rst_pars.rst.track_delay / reg.iter_period;

                regErrInitDelay(&reg.i_err, calloc(1+(size_t)delay_in_iterations, sizeof(float)),
                                 reg_pars.i_rst_pars.rst.track_delay, reg.iter_period);

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
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
int32_t main(int argc, char **argv)
/*---------------------------------------------------------------------------------------------------------*/
{
    // Read parameters from files according to command line options

    ccparsGet(argc, argv);

    // Initialise iteration period

    reg.iter_period = ccpars_global.iter_period;

    // Prepare load model (must be before PrepareFunction() if FG_LIMITS is enabled)

    PrepareLoad();

    // Prepare function to be generated

    PrepareFunction();

    // Prepare limits for the measurement, reference and regulation error if required

    PrepareLimits();

    // Prepare to simulate load and voltage source if required

    PrepareSimulation();

    // Prepare to regulate field or current if required

    PrepareRegulation();

    // Generate report of parameter values and print to stderr if verbose flag (-v) was set

    ccparsGenerateReport();

    // Enable signals to be written to stdout

    ccsigsPrepare();

    // Run the test

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        // Generate function and simulate voltage source and load and regulate if required

        ccrunSimulation(ccpars_global.function);
    }
    else
    {
        // Generate function only - no simulation: this is just to test libfg

        ccrunFunGen(ccpars_global.function);
    }

    // Write FLOT data if output format is FLOT

    ccsigsFlot();

    // Program completed successfully

    exit(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*\
  End of file: cctest.c
\*---------------------------------------------------------------------------------------------------------*/

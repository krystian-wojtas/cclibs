/*---------------------------------------------------------------------------------------------------------*\
  File:     ccRun.c                                                                     Copyright CERN 2014

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

  Purpose:  This file contains the function that run the test as a function of time in one
            of three modes:

                - Simulation of voltage source and load with voltage, current or field regulation
                - Normal (increasing) time function generation tests
                - Reverse (descreasing) time function generation tests
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>

// Include cctest program header files

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRef.h"
#include "ccSigs.h"
#include "ccRun.h"

    // If voltage source will be simulated

/*    if(ccpars_global.sim_load == CC_ENABLED)
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

            reg.ref_rst = reg.ref_limited = ccrun.fg_meta[0].range.start;

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
                ccTestPrintError("RST regulator failed to initialise: S[0] is less than 1.0E-10");
                return(EXIT_FAILURE);
            }

            // Estimate the ref advance time

            regCalcRefAdvance(&reg, &reg_pars);
        }
    }*/


///*---------------------------------------------------------------------------------------------------------*/
//static void ccRunAbort(double time)
///*---------------------------------------------------------------------------------------------------------*\
//  This will initialise a RAMP function that will take over the from the running function and will smoothly
//  ramp to the minimum reference value given in the limits. This is only supported when regulating current
//  or field.  In this example, the rate of change is calculated from the ref values in the RST history.
//\*---------------------------------------------------------------------------------------------------------*/
//{
//    struct fg_ramp_config  config;
//
//    // Set up RAMP configuration from limits (either current or field according to mode)
//
//    config.final        = ccrun.fg_limits->min;
//    config.linear_rate  = ccrun.fg_limits->rate;
//    config.acceleration = ccrun.fg_limits->acceleration;
//
//    // If acceleration limit is not set then base the acceleration limit on the rate limit
//
//    if(config.acceleration <= 0.0)
//    {
//        config.acceleration = 10.0 * config.linear_rate / reg.period;
//    }
//
//    // Make ramp symmetric with deceleration = acceleration
//
//    config.deceleration = config.acceleration;
//
//    // Initialise a RAMP to take over the running function.  This will update fg_meta.duration which
//    // controls the length of the run in ccrunSimulation()
//
//    fgRampCalc(&config,
//               &ccpars_ramp.ramp_pars,
//                ccrun.reg_time,                                            // time of last RST calculation
//                regRstPrevRef(&reg.rst_vars),                              // last reference value
//                regRstDeltaRef(&reg.rst_vars) / reg.period,                // last reference rate
//               &fg_meta);
//
//    // Check that abort duration is not too large (limit to 50000 iterations)
//    // This can be a problem if the exponential decay is included and the exp_final parameter is too close
//    // to zero.
//
//    if(((fg_meta.duration - time) / reg.iter_period) > 50000)
//    {
//        fprintf(stderr,"Error : Aborting requires more than 50000 iterations : duration = %.1f\n",fg_meta.duration);
//        exit(EXIT_FAILURE);
//    }
//
//    ref_function_type = FG_RAMP;
//}
/*---------------------------------------------------------------------------------------------------------*/
static void ccRunStartFunction(uint32_t func_idx)
/*---------------------------------------------------------------------------------------------------------*\
  This function tests the ability to open and re-close the loop regulating current or field.  The user can
  specify the time and duration of a period of open-loop to be included with-in a run in closed loop.
\*---------------------------------------------------------------------------------------------------------*/
{
    ccrun.func_duration = ccrun.fg_meta[func_idx].duration;
    ccrun.fgen_func     = funcs[ccpars_global.function[func_idx]].fgen_func;
    ccrun.fg_pars       = funcs[ccpars_global.function[func_idx]].fg_pars;

    regSetMode(&reg, &reg_pars, ccpars_global.reg_mode[func_idx]);

    ccrun.ref_advance[func_idx] = reg.ref_advance;

    ccSigsStoreCursor(CSR_FUNC,function_type[func_idx].string);
}
/*---------------------------------------------------------------------------------------------------------*/
void ccRunSimulation(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will run a simulation of the voltage source and load. Regulation can be disabled (VOLTAGE)
  or enabled (FIELD or CURRENT). Multiple reference functions can be played in a sequence.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    iteration_idx   = 0;        // Iteration index
    uint32_t    func_run_f      = 1;        // Function running flag
    uint32_t    abort_f         = 0;        // Abort function flag
    float       ref;                        // Function generator reference value
    float       perturb_volts   = 0.0;      // Voltage perturbation to apply to circuit
    double      time            = 0.0;      // Iteration time (since start of run)
    double      ref_time        = 0.0;      // Reference time (since start of function)
    double      func_start_time = 0.0;      // Function start time (relative to start of run)

    // Prepare to generation the first function

    ccrun.func_idx = 0;

    ccRunStartFunction(0);

    ref_time = reg.ref_advance;

    // Loop until all functions have completed

    for(;;)
    {
        // Set measurements to simulated values

        regSetMeas(&reg, &reg_pars,
                   0.0,         // v_meas from ADC
                   0.0,         // i_meas from ADC
                   0.0,         // b_meas from field measurement system
                   1);          // sim_meas_control: 0=Use real measurements, 1=Use simulated measurements

        // If converter has not tripped then generate reference value using libfg function

        if(ccrun.vs_tripped_flag == 0)
        {
            func_run_f = ccrun.fgen_func(ccrun.fg_pars, &ref_time, &ref);
        }

        // Regulate converter - this returns 1 on iterations when the current or field regulation
        // algorithm is executed.

        if(regConverter(&reg,
                        &reg_pars,
                        &ref,                           // V_REF, I_REF or B_REF according to reg.mode
                        ccrun.feedforward_v_ref,        // V_REF when feedforward_control is 1
                        ccrun.feedforward_control,      // 1=Feed-forward  0=Feedback
                        1                               // max_abs_err_control (always enabled)
                        ) == 1)
        {
            // Record time of iterations when current or field regulation algorithm runs.
            // This is used by START function and ABORT to initialise a new PLEP.

            ccrun.reg_time = time;

           // Check for function abort based on the abort time

            if(abort_f == 0 && time >= ccpars_global.abort_time)
            {
//                ccrunAbort(time);

                abort_f = 1;
            }
        }

        // Apply voltage perturbation from the specified time

        if(time >= ccpars_load.perturb_time && perturb_volts == 0.0)
        {
            perturb_volts = ccpars_load.perturb_volts;
        }

        // Store and print to CSV file the enabled signals

        ccSigsStore(time);

        // Simulate voltage source and load response (with voltage perturbation added)

        regSimulate(&reg, &reg_pars, perturb_volts);

        // Check if any condition requires the converter to trip

        if(ccrun.vs_tripped_flag       == 0 &&
          (reg.lim_b_meas.flags.trip   == 1 ||
           reg.lim_i_meas.flags.trip   == 1 ||
           reg.b_err.fault.flag        == 1 ||
           reg.i_err.fault.flag        == 1 ||
           reg.v_err.fault.flag        == 1))
        {
            // Simulate converter trip - switch to voltage regulation mode and set v_ref to zero

            ccrun.vs_tripped_flag = 1;

            regSetMode(&reg, &reg_pars, REG_VOLTAGE);

            ref = reg.ref = reg.ref_limited = reg.ref_rst = reg.v_ref = reg.v_ref_sat = reg.v_ref_limited = 0.0;

            ccSigsStoreCursor(CSR_FUNC,"TRIP!");
        }

        // Calculate next iteration time

        time = reg.iter_period * ++iteration_idx;

        ref_time = time - func_start_time + reg.ref_advance;

        // If reference function has finished

        if(ref_time >= ccrun.func_duration && func_run_f == 0)
        {
            // If not yet into the stop delay period, advance to next function

            if(ccrun.func_idx < ccrun.num_functions)
            {
                // If another function is in the list

                if(++ccrun.func_idx < ccrun.num_functions)
                {
                    // Prepare to generate the new function

                    func_start_time += ccrun.func_duration + ccpars_global.pre_func_delay;

                    ccRunStartFunction(ccrun.func_idx);

                    ref_time = time - func_start_time + reg.ref_advance;
                }
                else // else last function completed
                {
                    // Set duration to the stop delay and continue to use the last function

                    ccrun.func_duration += ccpars_global.stop_delay;
                }
            }
            else // else stop delay is complete so break out of loop
            {
                break;
            }
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccRunFuncGen(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will test the function generator with one or more functions, with increasing time only.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    iteration_idx   = 0;        // Iteration index
    uint32_t    func_run_f      = 1;        // Function running flag
    double      time            = 0.0;      // Iteration time (since start of run)
    double      ref_time        = 0.0;      // Reference time (since start of function)
    double      func_start_time = 0.0;      // Function start time (relative to start of run)

    // Prepare to generation the first function

    ccrun.func_idx      = 0;
    ccrun.func_duration = ccrun.fg_meta[0].duration;
    ccrun.fgen_func     = funcs[ccpars_global.function[0]].fgen_func;
    ccrun.fg_pars       = funcs[ccpars_global.function[0]].fg_pars;

    // Loop until all functions have completed

    for(;;)
    {
        // Generate reference value using libfg function

        func_run_f = ccrun.fgen_func(ccrun.fg_pars, &ref_time, &reg.v_ref);

        // Store and print to CSV file the enabled signals

        ccSigsStore(time);

        // Calculate next iteration time

        time = reg.iter_period * ++iteration_idx;

        ref_time = time - func_start_time;

        // If reference function has finished

        if(ref_time >= ccrun.func_duration && func_run_f == 0)
        {
            // If not yet into the stop delay period, advance to next function

            if(ccrun.func_idx < ccrun.num_functions)
            {
                // If another function is in the list

                if(++ccrun.func_idx < ccrun.num_functions)
                {
                    // Prepare to generate the new function

                    func_start_time    += ccrun.func_duration + ccpars_global.pre_func_delay;
                    ccrun.func_duration = ccrun.fg_meta[ccrun.func_idx].duration;
                    ccrun.fgen_func     = funcs[ccpars_global.function[ccrun.func_idx]].fgen_func;
                    ccrun.fg_pars       = funcs[ccpars_global.function[ccrun.func_idx]].fg_pars;
                    ref_time            = time - func_start_time;
                }
                else // else last function completed
                {
                    // Set duration to the stop delay and continue to use the last function

                    ccrun.func_duration += ccpars_global.stop_delay;
                }
            }
            else // else stop delay is complete so break out of loop
            {
                break;
            }
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccRunFuncGenReverseTime(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will test a single function with decreasing sample time.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    iteration_idx;          // Iteration index
    double      time;                   // Iteration time (runs backwards)

    // Reverse time can only be used with a single function

    ccrun.func_duration = ccrun.fg_meta[0].duration + ccpars_global.stop_delay;

    ccrun.num_iterations = (uint32_t)(1.4999 + ccrun.func_duration / reg.iter_period);

    ccrun.fgen_func = funcs[ccpars_global.function[0]].fgen_func;
    ccrun.fg_pars   = funcs[ccpars_global.function[0]].fg_pars;

    // Loop for the duration of the function plus stop delay

    for(iteration_idx = 0 ; iteration_idx < ccrun.num_iterations ; iteration_idx++)
    {
        // Calculate iteration time

        time = reg.iter_period * (ccrun.num_iterations - iteration_idx - 1);

        // Generate reference value using libfg function

        ccrun.fgen_func(ccrun.fg_pars, &time, &reg.v_ref);

        // Store and print enabled signals

        ccSigsStore(time);
    }
}
// EOF

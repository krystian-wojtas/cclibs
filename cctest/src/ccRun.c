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


/*---------------------------------------------------------------------------------------------------------
static void ccRunAbort(float time)
/*---------------------------------------------------------------------------------------------------------*\
  This will initialise a RAMP function that will take over the from the running function and will smoothly
  ramp to the minimum reference value given in the limits. This is only supported when regulating current
  or field.  In this example, the rate of change is calculated from the ref values in the RST history.
\*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_ramp_config  config;

    // Set up RAMP configuration from limits (either current or field according to mode)

    config.final        = ccpars_limits.fg->min;
    config.linear_rate  = ccpars_limits.fg->rate;
    config.acceleration = ccpars_limits.fg->acceleration;

    // If acceleration limit is not set then base the acceleration limit on the rate limit

    if(config.acceleration <= 0.0)
    {
        config.acceleration = 10.0 * config.linear_rate / reg.period;
    }

    // Make ramp symmetric with deceleration = acceleration

    config.deceleration = config.acceleration;

    // Initialise a RAMP to take over the running function.  This will update fg_meta.duration which
    // controls the length of the run in ccrunSimulation()

    fgRampCalc(&config,
               &ccpars_ramp.ramp_pars,
                ccrun.reg_time,                                            // time of last RST calculation
                regRstPrevRef(&reg.rst_vars),                              // last reference value
                regRstDeltaRef(&reg.rst_vars) / reg.period,                // last reference rate
               &fg_meta);

    // Check that abort duration is not too large (limit to 50000 iterations)
    // This can be a problem if the exponential decay is included and the exp_final parameter is too close
    // to zero.

    if(((fg_meta.duration - time) / reg.iter_period) > 50000)
    {
        fprintf(stderr,"Error : Aborting requires more than 50000 iterations : duration = %.1f\n",fg_meta.duration);
        exit(EXIT_FAILURE);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static float ccRunTestOpeningLoop(float time, float ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function tests the ability to open and re-close the loop regulating current or field.  The user can
  specify the time and duration of a period of open-loop to be included with-in a run in closed loop.
\*---------------------------------------------------------------------------------------------------------*/
{
    // If time is within period for open-loop operation

    if(time >= ccpars_global.open_loop_time && time < ccrun.close_loop_time)
    {
        // If still closed loop then switch to voltage mode and set a cursor in the log

        if(reg.mode != REG_VOLTAGE)
        {
            regSetVoltageMode(&reg, &reg_pars);
            ccsigsStoreCursor(CSR_REGMODE,"Open-loop");
        }

        // Force reference to hold the voltage reference produced by regSetVoltageMode()

        ref = reg.v_ref;
    }

    // else if time is after period for open-loop operation then re-close the loop
    //     Note: in this case the rate of change is assumed to be zero but it is also possible
    //     to close the loop with a non-zero rate of change provided it can be measured and given
    //     to the regSetMode() function.

    else if(time >= ccrun.close_loop_time && reg.mode == REG_VOLTAGE)
    {
        if(ccpars_global.reg_mode == REG_CURRENT)
        {
            regSetMode(&reg, &reg_pars, REG_CURRENT, reg.i_meas.unfiltered, 0.0);
        }
        else
        {
            regSetMode(&reg, &reg_pars, REG_FIELD,   reg.b_meas.unfiltered, 0.0);
        }

        ccsigsStoreCursor(CSR_REGMODE,"Close-loop");
    }

    return(ref);
}
/*---------------------------------------------------------------------------------------------------------*/
static float ccRunTestForConverterTrip(float ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function checks the limit fault flags and sets the converter trip flag if any critical limit has
  been exceeded.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(ccpars_vs.trip_flag         == 0 &&
      (reg.lim_b_meas.flags.trip   == 1 ||
       reg.lim_i_meas.flags.trip   == 1 ||
       reg.b_err.fault.flag        == 1 ||
       reg.i_err.fault.flag        == 1 ||
       reg.v_err.fault.flag        == 1))
    {
        // Simulate converter trip - switch to voltage regulation mode and set v_ref to zero

        ccpars_vs.trip_flag = 1;

        regSetVoltageMode(&reg, &reg_pars);

        ref = reg.ref = reg.ref_limited = reg.ref_rst = reg.v_ref = reg.v_ref_sat = reg.v_ref_limited = 0.0;
    }

    return(ref);
}
/*---------------------------------------------------------------------------------------------------------*/
void ccRunSimulation(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will run a simulation of the voltage source and load.  Regulation maybe enabled (AMPS or
  GAUSS) or disabled (VOLTAGE).
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    abort_f       = 0;    // Abort function flag
    uint32_t    func_run_f    = 1;    // Function running flag
    uint32_t    func_idx      = 0;    // Function index
    uint32_t    iteration_idx = 0;    // Iteration index
    float       perturb_volts = 0.0;  // Voltage perturbation to apply to circuit
    double      time;                 // Iteration time
    double      start_func_time;      // Time of the start of the function
    double      ref_time;             // Reference function time
    double      end_time = 0.0;       // Simulation end time
    float       ref;                  // Function generator reference value

    // Prepare o


    while(end_time == 0.0 || time < end_time)
    {
        // Calculate iteration time

        time = reg.iter_period * iteration_idx++;

        // Set measurements to simulated values

        regSetMeas(&reg, &reg_pars,
                   0.0,         // v_meas from ADC
                   0.0,         // i_meas from ADC
                   0.0,         // b_meas from field measurement system
                   1);          // sim_meas_control: 0=Use real measurements, 1=Use simulated measurements

        // Check for function abort based on the abort time

        if(abort_f == 0 && time >= ccpars_global.abort_time)
        {
            ccrunAbort(time);

            abort_f = 1;
            ref_function_type = FG_RAMP;
        }

        // If converter has not tripped then generate reference value using libfg function

        if(ccpars_vs.trip_flag == 0)
        {
            ref_time = time + reg.ref_advance;

            func_run_f = funcs[ref_function_type].fgen_func(funcs[ref_function_type].fg_pars, &ref_time, &ref);

            // if regulating current or field then open the loop at the specified time for the specified duration

            if(ccpars_global.reg_mode != REG_VOLTAGE)
            {
                ref = ccrunTestOpeningLoop(time, ref);
            }
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
        }

        // Apply voltage perturbation from the specified time

        if(time >= ccpars_load.perturb_time && perturb_volts == 0.0)
        {
            perturb_volts = ccpars_load.perturb_volts;
            ccsigsStoreCursor(CSR_LOAD,"Perturbation");
        }

        // Set end of simulation time when function stops running or converter trips

        if(end_time == 0.0 && (func_run_f == 0 || ccpars_vs.trip_flag == 1))
        {
            end_time = time + ccpars_global.stop_delay;
        }

        // Simulate voltage source and load response (with voltage perturbation added)

        regSimulate(&reg, &reg_pars, perturb_volts);

        // Store and print enabled signals

        ccsigsStore(time);

        // Check if measurement or regulation limits are exceeded that require a converter to be tripped off

        ref = ccrunTestForConverterTrip(ref);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccRunFuncGen(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will test the function generator with one or more functions, with increasing time only.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    iteration_idx   = 0;        // Iteration index
    uint32_t    func_idx        = 0;        // Function index
    uint32_t    func_run_f      = 1;        // Function running flag
    double      time            = 0.0;      // Iteration time (since start of run)
    double      ref_time        = 0.0;      // Reference time (since start of function)
    double      func_start_time = 0.0;      // Function start time (relative to start of run)
    float       func_duration;              // Duration of current function
    uint32_t  (*fgen_func)();               // Function to generate the reference
    void        *fg_pars;                   // Function generation parameter structure

    // Prepare to generation the first function

    func_duration = ccrun.fg_meta[0].duration;
    fgen_func     = funcs[ccpars_global.function[0]].fgen_func;
    fg_pars       = funcs[ccpars_global.function[0]].fg_pars;

    // Loop for until all funtions have completed

    for(;;)
    {
        // Generate reference value using libfg function

        func_run_f = fgen_func(fg_pars, &ref_time, &reg.v_ref);

        // Store and print to CSV file the enabled signals

        ccSigsStore(time);

        // Calculate next iteration time

        time = reg.iter_period * ++iteration_idx;

        ref_time = time - func_start_time;

        // If reference function has finished

        if(ref_time >= func_duration && func_run_f == 0)
        {
            // If not yet into the stop delay period, advance to next function

            if(func_idx < ccrun.num_functions)
            {
                func_idx++;

                // If another function is in the list

                if(func_idx < ccrun.num_functions)
                {
                    // Prepare to generate the new function

                    func_start_time += func_duration + ccpars_global.pre_func_delay;
                    func_duration    = ccrun.fg_meta[func_idx].duration;
                    fgen_func        = funcs[ccpars_global.function[func_idx]].fgen_func;
                    fg_pars          = funcs[ccpars_global.function[func_idx]].fg_pars;
                    ref_time         = time - func_start_time;
                }
                else // else last function completed
                {
                    // Set duration to the stop delay and continue to use the last function

                    func_duration += ccpars_global.stop_delay;
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
    float       duration;               // Run duration (function time and stop delay)
    uint32_t  (*fgen_func)();           // Function to generate the reference
    void        *fg_pars;               // Function generation parameter structure

    // Reverse time can only be used with a single function

    duration = ccrun.fg_meta[0].duration + ccpars_global.stop_delay;

    ccrun.num_iterations = (uint32_t)(1.4999 + duration / reg.iter_period);

    fgen_func = funcs[ccpars_global.function[0]].fgen_func;
    fg_pars   = funcs[ccpars_global.function[0]].fg_pars;

    // Loop for the duration of the function plus stop delay

    for(iteration_idx = 0 ; iteration_idx < ccrun.num_iterations ; iteration_idx++)
    {
        // Calculate iteration time

        time = reg.iter_period * (ccrun.num_iterations - iteration_idx - 1);

        // Generate reference value using libfg function

        fgen_func(fg_pars, &time, &reg.v_ref);

        // Store and print enabled signals

        ccSigsStore(time);
    }
}
// EOF

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
#include <math.h>

// Include cctest program header files

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRef.h"
#include "ccSigs.h"
#include "ccRun.h"

/*---------------------------------------------------------------------------------------------------------*/
static uint32_t ccRunAbort(double iter_time)
/*---------------------------------------------------------------------------------------------------------*\
  This will initialize a RAMP function that will take over the from the running function and will smoothly
  ramp to the minimum reference value given in the limits. This is only supported when regulating current
  or field.  In this example, the rate of change is calculated from the ref values in the RST history.
\*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_ramp_config  config;
    struct fg_meta         meta;

    // Set up RAMP configuration from limits (either current or field according to mode)

    config.final        = 0.0;
    config.linear_rate  = ccrun.fg_limits->rate;
    config.acceleration = ccrun.fg_limits->acceleration;

    // If acceleration limit is not set then base the acceleration limit on the rate limit

    if(config.acceleration <= 0.0)
    {
        config.acceleration = config.linear_rate / (10.0 * conv.reg_period);
    }

    // Make ramp symmetric with deceleration = acceleration

    config.deceleration = config.acceleration;

    // Initialize a RAMP to take over the running function.

    fgRampCalc(&config,
               0.0,
               regRstPrevRefRT (&conv.reg_signal->rst_vars),                    // last reference value
               regRstDeltaRefRT(&conv.reg_signal->rst_vars) / conv.reg_period,  // last reference rate
               &ccpars_ramp.pars,
               &meta);

    // Check that abort duration is not too large (limit to 50000 iterations)
    // This can be a problem if the exponential decay is included and the exp_final parameter is too close
    // to zero.

    if(((meta.duration - iter_time) / conv.iter_period) > 50000)
    {
        ccTestPrintError("aborting requires more than 50000 iterations : duration = %.1f\n",meta.duration);
        return(EXIT_FAILURE);
    }

    ccrun.func_start_time = iter_time + conv.ref_advance;

    ccrun.fgen_func = funcs[FG_RAMP].fgen_func;
    ccrun.fg_pars   = funcs[FG_RAMP].fg_pars;

    // Cancel any subsequent functions

    if(ccrun.pre_func_idx == 0)
    {
        ccrun.num_functions = ccrun.func_idx + 1;
    }
    else
    {
        ccrun.num_functions = ccrun.func_idx;
    }
    ccrun.pre_func_idx  = 0;

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
static uint32_t ccRunStartFunction(double iter_time, float *ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function tests the ability to open and re-close the loop regulating current or field.  The user can
  specify the time and duration of a period of open-loop to be included with-in a run in closed loop.
\*---------------------------------------------------------------------------------------------------------*/
{
    float           delay;
    float           rate;
    struct fg_meta  meta;

    // If a pre-function RAMP segment should be started

    if(ccrun.pre_func_idx < ccrun.num_prefunc_ramps)
    {
        // If first pre-function segment

        if(ccrun.pre_func_idx == 0)
        {
            // Log max abs error for previous function

            switch(conv.reg_mode)
            {
            case REG_FIELD  : ccrun.func[ccrun.func_idx].max_abs_err = conv.b.err.max_abs_err; break;
            case REG_CURRENT: ccrun.func[ccrun.func_idx].max_abs_err = conv.i.err.max_abs_err; break;
            default:          ccrun.func[ccrun.func_idx].max_abs_err = 0.0;                    break;
            }

            // If functions are finished then program a final plateau of duration GLOBAL STOP_DELAY

            if(++ccrun.func_idx >= ccrun.num_functions)
            {
                delay = ccpars_global.stop_delay;
                rate  = 0.0;

                ccrun.num_prefunc_ramps = 1;
                ccrun.pre_func_final_ref[0] = *ref;
            }
            else // else start pre-function between two functions
            {
                float invert_limits = conv.reg_signal->lim_ref.invert_limits == REG_ENABLED ? -1.0 : 1.0;

                // Set regulation mode for the next function

                regConvSetModeRT(&conv, ccpars_global.reg_mode[ccrun.func_idx]);

                // Set up pre-function segment references according to the pre-function policy

                switch(ccpars_prefunc.policy)
                {
                    case PREFUNC_RAMP:

                        ccrun.num_prefunc_ramps     = 1;
                        ccrun.pre_func_final_ref[0] = ccrun.func[ccrun.func_idx].fg_meta.range.start;
                        break;

                    case PREFUNC_STOPSTART:

                        ccrun.num_prefunc_ramps     = 2;
                        ccrun.pre_func_final_ref[0] = 0.0;
                        ccrun.pre_func_final_ref[1] = ccrun.func[ccrun.func_idx].fg_meta.range.start;
                        break;

                    case PREFUNC_MIN:

                        ccrun.num_prefunc_ramps     = 2;
                        ccrun.pre_func_final_ref[0] = conv.lim_ref->min * invert_limits;
                        ccrun.pre_func_final_ref[1] = ccrun.func[ccrun.func_idx].fg_meta.range.start;
                        break;

                    case PREFUNC_MINMAX:

                        ccrun.num_prefunc_ramps     = 3;
                        ccrun.pre_func_final_ref[0] = conv.lim_ref->min * invert_limits;
                        ccrun.pre_func_final_ref[1] = conv.lim_ref->pos * invert_limits;
                        ccrun.pre_func_final_ref[2] = ccrun.func[ccrun.func_idx].fg_meta.range.start;
                        break;
                }

                delay = 0;

                if(conv.reg_mode == REG_VOLTAGE)
                {
                    *ref = regRstPrevActRT (&conv.reg_signal->rst_vars);
                    rate = regRstAverageDeltaActRT(&conv.reg_signal->rst_vars) / conv.reg_period;
                }
                else
                {
                    *ref = regRstPrevRefRT (&conv.reg_signal->rst_vars);
                    rate = conv.reg_signal->rate.estimate;
                }
           }

            // Prepare to generate the first pre-function RAMP segment


            ccrun.fgen_func = fgRampGen;
            ccrun.fg_pars   = &ccpars_prefunc.pars;
        }
        else // Prepare to generate the second or third pre-function RAMP segment
        {
            delay = ccpars_prefunc.plateau_duration;
            rate  = 0.0;
        }

        // Arm a RAMP for the next pre-function segment - flip reference sign when limits are inverted

        ccpars_prefunc.config.final = ccrun.pre_func_final_ref[ccrun.pre_func_idx];

        fgRampCalc(&ccpars_prefunc.config,
                   delay,
                   *ref,
                   rate,
                   &ccpars_prefunc.pars,
                   &meta);

        ccrun.func_start_time = iter_time + conv.ref_advance;

        if(conv.reg_mode != REG_VOLTAGE)
        {
            ccrun.func_start_time -= conv.reg_signal->iteration_counter * conv.iter_period;
        }
        ccrun.pre_func_idx++;
    }
    else // Pre-function sequence complete - start the next function
    {
        // If all functions completed

        if(ccrun.func_idx >= ccrun.num_functions)
        {
            return(0);      // Return 0 to end simulation
        }

        // Set regulation mode (which won't change) to reset max abs error values

        regConvSetModeRT(&conv, ccpars_global.reg_mode[ccrun.func_idx]);

        // Prepare to generate new function

        ccrun.fgen_func = funcs[ccpars_global.function[ccrun.func_idx]].fgen_func;
        ccrun.fg_pars   = funcs[ccpars_global.function[ccrun.func_idx]].fg_pars;

        ccrun.func_start_time = iter_time;

        ccrun.func[ccrun.func_idx].ref_advance     = conv.ref_advance;
        ccrun.func[ccrun.func_idx].func_start_time = iter_time;

        ccSigsStoreCursor(CSR_FUNC,function_type[ccrun.func_idx].string);

        // Reset pre-function index for when this new function ends

        ccrun.pre_func_idx      = 0;
        ccrun.num_prefunc_ramps = 1;

        // Reset table segment index in case function is DIRECT

        ccpars_table.pars.seg_idx = 0;
    }

    return(1); // Return 1 to continue the simulation
}
/*---------------------------------------------------------------------------------------------------------*/
void ccRunSimulation(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will run a simulation of the voltage source and load. Regulation can be disabled (VOLTAGE)
  or enabled (FIELD or CURRENT). Multiple reference functions can be played in a sequence.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t        iteration_idx     = 0;        // Iteration index
    bool            abort             = false;    // Abort function flag
    bool            func_running      = true;     // Function running flag
    float           ref;                          // Function generator reference value
    float           perturb_volts     = 0.0;      // Voltage perturbation to apply to circuit
    double          iter_time         = 0.0;      // Iteration time (since start of run)
    double          ref_time          = 0.0;      // Reference time (since start of function)
    double          trip_time         = 0.0;

    // Prepare to generation the first function with no pre-function

    ccrun.pre_func_idx      = 0;
    ccrun.num_prefunc_ramps = 0;
    ccrun.func_idx          = 0;
    ccrun.vs_tripped_flag   = 0;

    // Call once with conv.reg_mode equal REG_NONE to set iteration counters

    regConvSetMeasRT(&conv, REG_OPERATIONAL_RST_PARS, 0, 0, 1);

    ref = ccrun.func[0].fg_meta.range.start;

    ccRunStartFunction(iter_time, &ref);

    ref_time = conv.ref_advance;

    // Loop until all functions have completed

    for(;;)
    {
        uint32_t        good_meas_flag;             // Good measurement flag
        uint32_t        reg_iteration_counter;      // Regulation iteration counter from libreg (0=start of reg period)

        // Calculate reference time taking into account the ref advance for the active regulation mode

        ref_time = iter_time - ccrun.func_start_time + conv.ref_advance;

        // Set measurements to simulated values but support bad values at a given period
        // The "real" measurements are invalid while the simulated measurements are good

        good_meas_flag = ccpars_meas.invalid_meas_period_iters == 0 ||
                     (iteration_idx % ccpars_meas.invalid_meas_period_iters) >= ccpars_meas.invalid_meas_repeat_iters;

        // Start new iteration by processing the measurements

        reg_iteration_counter = regConvSetMeasRT(&conv, REG_OPERATIONAL_RST_PARS, 0, 0, good_meas_flag);

        // If converter has not tripped

        if(ccrun.vs_tripped_flag == 0)
        {
            // If this is the first iteration of the regulation period then calculate a new reference value

            if(reg_iteration_counter == 0)
            {
                func_running = ccrun.fgen_func(ccrun.fg_pars, &ref_time, &ref);
            }

            // Call ConvRegulate every iteration to keep RST histories up to date

            regConvRegulateRT(&conv, &ref);

            // If this is the first iteration of the regulation period then check for function abort or function end

            if(reg_iteration_counter == 0)
            {
                // If function should be aborted

                if(abort == false && ccpars_global.abort_time > 0.0 && iter_time >= ccpars_global.abort_time)
                {
                    abort = true;

                    if(ccRunAbort(iter_time) == EXIT_FAILURE)
                    {
                        break;
                    }
                }

                // else if function has finished

                else if(func_running == false)
                {
                    // Starting a new function can change conv.reg_mode, but not our local copy of reg_mode

                    if(ccRunStartFunction(iter_time, &ref) == 0)
                    {
                        break;
                    }
                }
            }
        }

        // Apply voltage reference quantization if specified

        if(ccpars_vs.quantization > 0.0)
        {
            conv.v.ref_limited = ccpars_vs.quantization * nearbyintf(conv.v.ref_limited / ccpars_vs.quantization);
        }

        // Apply voltage perturbation from the specified time

        if(iter_time >= ccpars_load.perturb_time && perturb_volts == 0.0)
        {
            perturb_volts = ccpars_load.perturb_volts;
        }

        // Simulate voltage source and load response (with voltage perturbation added)

        regConvSimulateRT(&conv, perturb_volts);

        // Check if simulated converter should be trip

        if(ccrun.vs_tripped_flag == 0)
        {
            if(conv.b.lim_meas.flags.trip      == 1 ||
               conv.i.lim_meas.flags.trip      == 1 ||
               conv.lim_i_rms.flags.fault      == 1 ||
               conv.lim_i_rms_load.flags.fault == 1 ||
               conv.b.err.fault.flag           == 1 ||
               conv.i.err.fault.flag           == 1 ||
               conv.v.err.fault.flag           == 1)
            {
                // Simulate converter trip by switching to regulation mode to NONE

                trip_time = iter_time;
                ccrun.vs_tripped_flag = 1;

                regConvSetModeRT(&conv, REG_NONE);

                ccSigsStoreCursor(CSR_FUNC,"TRIP!");

                puts("Trip");

                // Cancel any subsequent functions

                if(ccrun.pre_func_idx == 0)
                {
                    ccrun.num_functions = ccrun.func_idx + 1;
                }
                else
                {
                    ccrun.num_functions = ccrun.func_idx;
                }
            }
        }
        else if((iter_time - trip_time) > ccpars_global.stop_delay)
        {
            // After a trip, stop after STOP_DELAY

            break;
        }

        // Store and print to CSV file the enabled signals

        ccSigsStore(iter_time);

        // Calculate next iteration time

        iter_time = conv.iter_period * ++iteration_idx;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccRunFuncGen(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will test the function generator with one or more functions, with increasing time only.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    iteration_idx   = 0;        // Iteration index
    bool        func_running    = true;     // Function running flag
    double      iter_time       = 0.0;      // Iteration time (since start of run)
    double      ref_time;                   // Reference time (since start of function)
    double      func_start_time;            // Function start time (relative to start of run)

    // Prepare to generation the first function

    ccrun.func_idx      = 0;
    ccrun.func_duration = ccpars_global.run_delay + ccrun.func[0].fg_meta.duration;
    ccrun.fgen_func     = funcs[ccpars_global.function[0]].fgen_func;
    ccrun.fg_pars       = funcs[ccpars_global.function[0]].fg_pars;

    ccrun.func[0].func_start_time = func_start_time = 0.0;

    // Loop until all functions have completed

    for(;;)
    {
        ref_time = iter_time - func_start_time;

        // Generate reference value using libfg function

        func_running = ccrun.fgen_func(ccrun.fg_pars, &ref_time, &conv.v.ref);

        // If reference function has finished

        if(ref_time > ccrun.func_duration && func_running == 0)
        {
            // If not yet into the stop delay period, advance to next function

            if(ccrun.func_idx < ccrun.num_functions)
            {
                // If another function is in the list

                if(++ccrun.func_idx < ccrun.num_functions)
                {
                    // Prepare to generate the new function

                    ccrun.func_duration = ccpars_global.run_delay + ccrun.func[ccrun.func_idx].fg_meta.duration;
                    ccrun.fgen_func     = funcs[ccpars_global.function[ccrun.func_idx]].fgen_func;
                    ccrun.fg_pars       = funcs[ccpars_global.function[ccrun.func_idx]].fg_pars;

                    ccrun.func[ccrun.func_idx].func_start_time = func_start_time = iter_time + ccpars_prefunc.plateau_duration;
                }
                else // else last function completed
                {
                    // Set duration to the stop delay and continue to use the last function

                    ccrun.func_duration = ref_time + ccpars_global.stop_delay;
                }
            }
            else // else stop delay is complete so break out of loop
            {
                break;
            }
        }

        // Store and print to CSV file the enabled signals

        ccSigsStore(iter_time);

        // Calculate next iteration time

        iter_time = conv.iter_period * ++iteration_idx;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccRunFuncGenReverseTime(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will test a single function with decreasing sample time.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    iteration_idx;          // Iteration index
    double      iter_time;              // Iteration time (runs backwards)

    // Reverse time can only be used with a single function

    ccrun.func_duration = ccpars_global.run_delay + ccrun.func[0].fg_meta.duration + ccpars_global.stop_delay;

    ccrun.num_iterations = (uint32_t)(1.4999 + ccrun.func_duration / conv.iter_period);

    ccrun.fgen_func = funcs[ccpars_global.function[0]].fgen_func;
    ccrun.fg_pars   = funcs[ccpars_global.function[0]].fg_pars;

    // Loop for the duration of the function plus stop delay

    for(iteration_idx = 0 ; iteration_idx < ccrun.num_iterations ; iteration_idx++)
    {
        // Calculate iteration time

        iter_time = conv.iter_period * (ccrun.num_iterations - iteration_idx - 1);

        // Generate reference value using libfg function

        ccrun.fgen_func(ccrun.fg_pars, &iter_time, &conv.v.ref);

        // Store and print enabled signals

        ccSigsStore(iter_time);
    }
}
// EOF

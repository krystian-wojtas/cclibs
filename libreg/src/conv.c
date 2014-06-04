/*---------------------------------------------------------------------------------------------------------*\
  File:     conv.c                                                                       Copyright CERN 2014

  License:  This file is part of libreg.

            libreg is free software: you can redistribute it and/or modify
            it under the terms of the GNU Lesser General Public License as published by
            the Free Software Foundation, either version 3 of the License, or
            (at your option) any later version.

            This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
            MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
            GNU Lesser General Public License for more details.

            You should have received a copy of the GNU Lesser General Public License
            along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Purpose:  This provides a higher level access to libreg with functions that combine all the elements
            needed to regulate current or field in converter.
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "libreg.h"

//-----------------------------------------------------------------------------------------------------------
// Non-Real-Time Functions - do not call these from the real-time thread or interrupt
//-----------------------------------------------------------------------------------------------------------
float regConvPureDelay(struct reg_conv *reg, struct reg_meas_filter *meas_filter, uint32_t reg_period_iters)
/*---------------------------------------------------------------------------------------------------------*\
  This function will estimate the pure loop delay in regulation periods needed by regRstInit().
\*---------------------------------------------------------------------------------------------------------*/
{
    return((reg->sim_vs_pars.v_ref_delay_iters + reg->sim_vs_pars.step_rsp_time_iters +
            meas_filter->delay_iters[meas_filter->reg_select]) / (float)reg_period_iters);
}
/*---------------------------------------------------------------------------------------------------------*/
void regConvInitSimLoad(struct reg_conv *reg, enum reg_mode reg_mode, float sim_load_tc_error)
/*---------------------------------------------------------------------------------------------------------*\
  This function will initialise the simulated load structures with the specified load parameters
\*---------------------------------------------------------------------------------------------------------*/
{
    regSimLoadTcError(&reg->sim_load_pars, &reg->load_pars, sim_load_tc_error);

    regSimLoadInit(&reg->sim_load_pars, reg->iter_period);

    switch(reg->mode = reg_mode)
    {
    default:

        regSimLoadSetVoltage(&reg->sim_load_pars, &reg->sim_load_vars, reg->v.meas);
        break;

    case REG_CURRENT:

        regSimLoadSetCurrent(&reg->sim_load_pars, &reg->sim_load_vars, reg->i.meas.signal[REG_MEAS_UNFILTERED]);
        break;

    case REG_FIELD:

        regSimLoadSetField(&reg->sim_load_pars, &reg->sim_load_vars, reg->b.meas.signal[REG_MEAS_UNFILTERED]);
        break;
    }

    reg->v.meas = reg->sim_load_vars.circuit_voltage;
    
    reg->i.meas.signal[REG_MEAS_FILTERED] = reg->i.meas.signal[REG_MEAS_UNFILTERED] = reg->sim_load_vars.circuit_current;
    reg->b.meas.signal[REG_MEAS_FILTERED] = reg->b.meas.signal[REG_MEAS_UNFILTERED] = reg->sim_load_vars.magnet_field;
}
//-----------------------------------------------------------------------------------------------------------
void regConvInitMeas(struct reg_conv *reg, struct reg_meas_signal *v_meas_p, struct reg_meas_signal *i_meas_p, struct reg_meas_signal *b_meas_p)
{
    reg->b_meas_p = b_meas_p;
    reg->i_meas_p = i_meas_p;
    reg->v_meas_p = v_meas_p;
}
//-----------------------------------------------------------------------------------------------------------
// Real-Time Functions
//-----------------------------------------------------------------------------------------------------------
void regConvSetMeasRT(struct reg_conv *reg, uint32_t use_sim_meas)
{
    if(use_sim_meas == 0)
    {
        // Use measured field, current and voltage measurement and status supplied by application

        reg->b_meas = *reg->b_meas_p;
        reg->i_meas = *reg->i_meas_p;
        reg->v_meas = *reg->v_meas_p;
    }
    else
    {
        // Use simulated measurements which are always OK

        reg->b_meas.signal = reg->b.sim.signal;
        reg->i_meas.signal = reg->i.sim.signal;
        reg->v_meas.signal = reg->v.sim.signal;

        reg->b_meas.status = REG_MEAS_SIGNAL_OK;
        reg->i_meas.status = REG_MEAS_SIGNAL_OK;
        reg->v_meas.status = REG_MEAS_SIGNAL_OK;
    }
}
//-----------------------------------------------------------------------------------------------------------
static void regConvSetModeVoltageRT(struct reg_conv *reg)
{
    // Initialise voltage references according to previous regulation mode

    switch(reg->mode)
    {
        case REG_FIELD:

            reg->v.ref = regRstAverageVrefRT(&reg->rst_vars);
            reg->v.ref_sat = reg->v.ref;
            break;

        case REG_CURRENT:

            reg->v.ref = regRstAverageVrefRT(&reg->rst_vars);
            reg->v.ref_sat = regLoadVrefSatRT(&reg->load_pars, reg->rst_vars.meas[0], reg->v.ref);
            break;

        default:    // NONE

            reg->v.ref_sat = reg->v.ref = 0.0;
            break;
    }

    reg->v.ref_limited = reg->v.ref_sat;

    // Calculate the ref advance for voltage mode

    reg->ref_advance = reg->iter_period *
                      (reg->sim_vs_pars.v_ref_delay_iters + reg->sim_vs_pars.step_rsp_time_iters);

    // Clear field and current regulation variables

    reg->ref         = 0.0;
    reg->ref_limited = 0.0;
    reg->ref_rst     = 0.0;
    reg->meas        = 0.0;
    reg->rst_vars.meas_track_delay_periods = 0.0;
    regErrResetLimitsVarsRT(&reg->i.err);
    regErrResetLimitsVarsRT(&reg->b.err);
}
//-----------------------------------------------------------------------------------------------------------
static void regConvSetModeFieldOrCurrentRT(struct reg_conv *reg, enum reg_mode reg_mode, uint32_t iteration_counter)
{
    uint32_t             idx;
    float                ref_offset;
    float                rate;
    float                v_ref;
    struct reg_conv_signal   *r;
    struct reg_rst_pars *rst_pars;
    struct reg_rst_vars *rst_vars = &reg->rst_vars;

    // If closing loop on current, adjust v_ref for magnet saturation assuming current is invariant.
    // This assumes it is unlikely that the current regulation will start with the current ramping
    // fast while deep into the magnet saturation zone.

    r = reg->r       = (reg_mode == REG_FIELD ? &reg->b : &reg->i);
    rst_pars         = &r->rst_pars;
    v_ref            = (reg->mode == REG_CURRENT ? regLoadInverseVrefSatRT(&reg->load_pars, reg->i.meas.signal[REG_MEAS_UNFILTERED], reg->v.ref_limited) : reg->v.ref_limited);
    rate             = (reg->mode != REG_NONE    ? r->rate.estimate : 0.0);
    reg->meas        = r->meas.signal[r->meas.reg_select] - rate * iteration_counter * reg->iter_period;
    reg->ref_advance = rst_pars->track_delay_periods * rst_pars->period - r->meas.delay_iters[r->meas.reg_select] * reg->iter_period;

    rst_pars->ref_delay_periods = rst_pars->track_delay_periods +
                                 (r->meas.delay_iters[REG_MEAS_UNFILTERED] - r->meas.delay_iters[r->meas.reg_select]) / (float)rst_pars->period_iters;
    regErrResetLimitsVarsRT(&r->err);

    // Prepare RST histories - assuming that v_ref has been constant when calculating rate

    reg->period            = rst_pars->period;
    reg->iteration_counter = iteration_counter;

    ref_offset  = rate * reg->ref_advance;

    for(idx = 0; idx < REG_N_RST_COEFFS; idx++)
    {
        rst_vars->act [idx] = v_ref;
        rst_vars->meas[idx] = reg->meas - rate * (float)(REG_N_RST_COEFFS - 1 - idx) * reg->period;
        rst_vars->ref [idx] = rst_vars->meas[idx] + ref_offset;
    }

    rst_vars->history_index     = idx - 1;

    reg->ref_delayed = regRstDelayedRefRT(&r->rst_pars, &reg->rst_vars, 0);


    reg->ref = reg->ref_limited = reg->ref_rst = rst_vars->ref[rst_vars->history_index];
}
//-----------------------------------------------------------------------------------------------------------
void regConvSetModeRT(struct reg_conv *reg, enum reg_mode reg_mode, uint32_t iteration_counter)
/*---------------------------------------------------------------------------------------------------------*\
  This function allows the regulation mode to be changed between voltage, current and field.
  This should be called at the start of an iteration before calling regConverter().
\*---------------------------------------------------------------------------------------------------------*/
{
    // If regulation mode has changed

    if(reg_mode != reg->mode)
    {
        // If switching to VOLTAGE mode

        if(reg_mode == REG_VOLTAGE)
        {
            regConvSetModeVoltageRT(reg);
        }
        else // else switching to FIELD or CURRENT regulation
        {
            regConvSetModeFieldOrCurrentRT(reg, reg_mode, iteration_counter);
        }

        // Store the new regulation mode

        reg->mode = reg_mode;
    }

    // Reset max abs error whenever regSetMode is called, even if the mode doesn't change

    reg->i.err.max_abs_err = reg->b.err.max_abs_err = 0.0;
}
//-----------------------------------------------------------------------------------------------------------
void regConvValidateMeas(struct reg_conv *reg)
{
    if(reg->mode != REG_VOLTAGE)
    {
        reg->ref_delayed = regRstDelayedRefRT(&reg->r->rst_pars, &reg->rst_vars, reg->iteration_counter);
    }

    // Check voltage measurement

    if(reg->v_meas.status != REG_MEAS_SIGNAL_OK)
    {
        // If voltage measurement is invalid then use voltage source model instead

        reg->v.meas = reg->v.err.delayed_ref;
        reg->v_meas_invalid_counter++;
    }
    else
    {
        reg->v.meas = reg->v_meas.signal;
    }

    // Check current measurement

    if(reg->i_meas.status != REG_MEAS_SIGNAL_OK)
    {
        if(reg->mode == REG_CURRENT)
        {
            // If regulating current then use delayed ref adjusted by the regulation error as the measurement

            reg->i.meas.signal[REG_MEAS_UNFILTERED] = reg->ref_delayed - reg->i.err.err;
        }
        else
        {
            // If not regulating current then extrapolate previous value using the current rate of change

            reg->i.meas.signal[REG_MEAS_UNFILTERED] += reg->i.rate.estimate * reg->iter_period;
        }

        reg->i_meas_invalid_counter++;
    }
    else
    {
        reg->i.meas.signal[REG_MEAS_UNFILTERED] = reg->i_meas.signal;
    }

    // Check field measurement

    if(reg->b_meas.status != REG_MEAS_SIGNAL_OK)
    {
        if(reg->mode == REG_FIELD)
        {
            // If regulating field then use delayed ref adjusted by the regulation error as the measurement

            reg->b.meas.signal[REG_MEAS_UNFILTERED] = reg->ref_delayed - reg->b.err.err;
        }
        else
        {
            // If not regulating current then extrapolate previous value using the current rate of change

            reg->b.meas.signal[REG_MEAS_UNFILTERED] += reg->b.rate.estimate * reg->iter_period;
        }

        reg->b_meas_invalid_counter++;
    }
    else
    {
        reg->b.meas.signal[REG_MEAS_UNFILTERED] = reg->b_meas.signal;
    }

}
//-----------------------------------------------------------------------------------------------------------
uint32_t regConverterRT(struct reg_conv *reg,                 // Regulation structure
                        float           *ref,                 // Ref for voltage, current or field
                        float            feedforward_v_ref,   // Feedforward voltage reference
                        uint32_t         feedforward_control, // Feedforward enable/disable control
                        uint32_t         enable_max_abs_err)  // Enable max abs error calculation
/*---------------------------------------------------------------------------------------------------------*\
  This function will control a converter in either open-loop voltage mode, or closed-loop current or
  field regulation modes.  It returns non-zero on iterations when regulation is active (i.e. always for
  VOLTAGE mode).
\*---------------------------------------------------------------------------------------------------------*/
{
    struct reg_conv_signal *r = reg->r;            // Pointer to active regulation structure (reg->b or reg->i)

    // Return immediately when REG_NONE

    if(reg->mode == REG_NONE)
    {
        reg->v.ref = reg->v.ref_sat = reg->v.ref_limited = 0.0;
        return(0);
    }

    // New iteration - validate measurements before using them

    reg->iteration_counter++;

    regConvValidateMeas(reg);

    // Calculate and check the voltage regulation limits

    regErrCheckLimitsRT(&reg->v.err, 1, 1, reg->v.err.delayed_ref, reg->v.meas);

    // Check current measurement limits

    regLimMeasRT(&reg->i.lim_meas, reg->i.meas.signal[REG_MEAS_UNFILTERED]);

    // Check field measurement limits only when regulating field

    if(reg->mode == REG_FIELD)
    {
        regLimMeasRT(&reg->b.lim_meas, reg->b.meas.signal[REG_MEAS_UNFILTERED]);
    }

    // Calculate voltage reference limits for the measured current (V limits can depend on current)

    regLimVrefCalcRT(&reg->v.lim_ref, reg->i.meas.signal[REG_MEAS_UNFILTERED]);

    // Filter the field and current measurements and prepare to estimate measurement rate

    regMeasFilterRT(&reg->b.meas);
    regMeasRateRT  (&reg->b.rate,reg->b.meas.signal[REG_MEAS_FILTERED],reg->b.rst_pars.period,reg->b.rst_pars.period_iters);

    regMeasFilterRT(&reg->i.meas);
    regMeasRateRT  (&reg->i.rate,reg->i.meas.signal[REG_MEAS_FILTERED],reg->i.rst_pars.period,reg->i.rst_pars.period_iters);

    // If open-loop (voltage regulation) mode - apply voltage ref limits

    if(reg->mode == REG_VOLTAGE)
    {
        reg->v.ref = reg->v.ref_sat = *ref;              // Don't apply magnet saturation compensation

        reg->v.ref_limited = regLimRefRT(&reg->v.lim_ref, reg->iter_period, reg->v.ref, reg->v.ref_limited);

        reg->flags.ref_clip = reg->v.lim_ref.flags.clip;
        reg->flags.ref_rate = reg->v.lim_ref.flags.rate;

        *ref = reg->v.ref_limited;

        reg->iteration_counter = 0;
    }
    else  // else closed-loop on current or field
    {
        // Regulate current or field at the regulation period

        if(reg->iteration_counter >= r->rst_pars.period_iters)
        {
            float v_ref;
            float unfiltered_meas;

            reg->iteration_counter = 0;

            reg->ref        = *ref;
            reg->meas       = r->meas.signal[r->meas.reg_select];
            unfiltered_meas = reg->i.meas.signal[REG_MEAS_UNFILTERED];
            regRstIncHistoryIndexRT(&reg->rst_vars);

            if(feedforward_control == 0)
            {
                // Apply current reference clip and rate limits

                reg->ref_limited = regLimRefRT(&r->lim_ref, reg->period, reg->ref, reg->ref_limited);

                // Calculate voltage reference using RST algorithm

                reg->v.ref = regRstCalcActRT(&r->rst_pars, &reg->rst_vars, reg->ref_limited, reg->meas);

                // Calculate magnet saturation compensation when regulating current only

                reg->v.ref_sat = (reg->mode == REG_CURRENT ? regLoadVrefSatRT(&reg->load_pars, unfiltered_meas, reg->v.ref) : reg->v.ref);

                // Apply voltage reference clip and rate limits

                reg->v.ref_limited = regLimRefRT(&reg->v.lim_ref, reg->period, reg->v.ref_sat, reg->v.ref_limited);

                // If voltage reference has been clipped

                if(reg->v.lim_ref.flags.clip || reg->v.lim_ref.flags.rate)
                {
                    // Back calculate the new v_ref before the saturation compensation when regulating current only

                    v_ref = (reg->mode == REG_CURRENT ? regLoadInverseVrefSatRT(&reg->load_pars, unfiltered_meas, reg->v.ref_limited) : reg->v.ref_limited);

                    // Back calculate new current reference to keep RST histories balanced

                    reg->ref_rst = regRstCalcRefRT(&r->rst_pars, &reg->rst_vars, v_ref, reg->meas);

                    // Mark current reference as rate limited

                    r->lim_ref.flags.rate = 1;
                }
                else
                {
                    reg->ref_rst = reg->ref_limited;
                }

                reg->flags.ref_clip = r->lim_ref.flags.clip;
                reg->flags.ref_rate = r->lim_ref.flags.rate;
            }
            else
            {
              // Open-loop: Use feedforward_v_ref

                reg->flags.ref_clip = 0;
                reg->v.ref = feedforward_v_ref;

                // Calculate v_ref with saturation compensation applied when regulating current only

                reg->v.ref_sat = (reg->mode == REG_CURRENT ? regLoadVrefSatRT(&reg->load_pars, unfiltered_meas, feedforward_v_ref) : feedforward_v_ref);

                // Apply voltage reference limits

                reg->v.ref_limited = regLimRefRT(&reg->v.lim_ref, reg->period, reg->v.ref_sat, reg->v.ref_limited);

                // If v_ref was clipped then back calculate the new uncompensated v_ref

                if(reg->v.lim_ref.flags.clip || reg->v.lim_ref.flags.rate)
                {
                    v_ref = (reg->mode == REG_CURRENT ? regLoadInverseVrefSatRT(&reg->load_pars, unfiltered_meas, reg->v.ref_limited) : reg->v.ref_limited);
                    reg->flags.ref_rate = 1;
                }
                else
                {
                    v_ref = reg->v.ref;
                    reg->flags.ref_rate = 0;
                }

                // Back calculate the current reference that would produce this voltage reference

                reg->ref = reg->ref_limited = reg->ref_rst = regRstCalcRefRT(&r->rst_pars, &reg->rst_vars, v_ref, reg->meas);
            }

            regRstTrackDelayRT(&reg->rst_vars, reg->period,r->lim_ref.rate_clip);

            *ref = reg->ref_rst;
        }

        // Monitor regulation error using the delayed reference and the unfiltered measurement

        if(r->err_rate == REG_ERR_RATE_MEASUREMENT || reg->iteration_counter == 0)
        {
            regErrCheckLimitsRT(&r->err, !feedforward_control, enable_max_abs_err,
                                reg->ref_delayed, r->meas.signal[REG_MEAS_UNFILTERED]);
        }
    }

    // Return 1 if first iteration of the regulation period

    return(reg->iteration_counter == 0);
}
//-----------------------------------------------------------------------------------------------------------
void regConvSimulateRT(struct reg_conv *reg, float v_perturbation)
/*---------------------------------------------------------------------------------------------------------*\
  This function will simulate the voltage source and load and the measurements of the voltage, current
  and field. The voltage reference comes from reg->v.ref_limited which is calculated by calling
  regConverter().  A voltage perturbation can be included in the simulation via the v_perturbation parameter.
\*---------------------------------------------------------------------------------------------------------*/
{
    float v_circuit;      // Simulated v_circuit without V_REF_DELAY

    // Simulate voltage source response to v_ref without taking into account V_REF_DELAY

    v_circuit = regSimVsRT(&reg->sim_vs_pars, &reg->sim_vs_vars, reg->v.ref_limited);

    // Simulate load current and field in response to sim_advanced_v_circuit plus the perturbation

    regSimLoadRT(&reg->sim_load_pars, &reg->sim_load_vars, v_circuit + v_perturbation);

    // Use delays to estimate the measurement of the magnet's field and the circuit's current and voltage

    reg->b.sim.signal = regDelaySignalRT(&reg->b.sim.meas_delay, reg->sim_load_vars.magnet_field,
                                      reg->sim_load_pars.vs_undersampled_flag && reg->sim_load_pars.load_undersampled_flag);
    reg->i.sim.signal = regDelaySignalRT(&reg->i.sim.meas_delay, reg->sim_load_vars.circuit_current,
                                      reg->sim_load_pars.vs_undersampled_flag && reg->sim_load_pars.load_undersampled_flag);
    reg->v.sim.signal = regDelaySignalRT(&reg->v.sim.meas_delay, reg->sim_load_vars.circuit_voltage,
                                      reg->sim_load_pars.vs_undersampled_flag);

    // Store simulated voltage measurement without noise as the delayed ref for the v_err calculation

    reg->v.err.delayed_ref = reg->v.sim.signal;

    // Simulate noise and tone on simulated measurement of the magnet's field and the circuit's current and voltage

    reg->b.sim.signal += regMeasNoiseAndToneRT(&reg->b.sim.noise_and_tone);
    reg->i.sim.signal += regMeasNoiseAndToneRT(&reg->i.sim.noise_and_tone);
    reg->v.sim.signal += regMeasNoiseAndToneRT(&reg->v.sim.noise_and_tone);
}
// EOF

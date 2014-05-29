/*---------------------------------------------------------------------------------------------------------*\
  File:     reg.c                                                                       Copyright CERN 2014

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
            needed to regulate current or field.
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "libreg.h"

/*---------------------------------------------------------------------------------------------------------*/
void regSetSimLoad(struct reg_converter *reg, enum reg_mode reg_mode, float sim_load_tc_error)
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
/*---------------------------------------------------------------------------------------------------------*/
void regSetMeas(struct reg_converter *reg, float v_meas, float i_meas, float b_meas, uint32_t use_sim_meas)
/*---------------------------------------------------------------------------------------------------------*\
  This function will set the unfiltered measured values in the reg structure based on the sim_meas_control.
  When active, the measurements will be based on the voltage source and load simulation calculated by
  regSimulate().
\*---------------------------------------------------------------------------------------------------------*/
{
    if(use_sim_meas == 0)
    {
        // Use measured values for voltage, current and field

        reg->v.meas = v_meas;
        reg->i.meas.signal[REG_MEAS_UNFILTERED] = i_meas;
        reg->b.meas.signal[REG_MEAS_UNFILTERED] = b_meas;
    }
    else
    {
        // Use simulated measurements

        reg->v.meas = reg->v.sim.signal;
        reg->i.meas.signal[REG_MEAS_UNFILTERED] = reg->i.sim.signal;
        reg->b.meas.signal[REG_MEAS_UNFILTERED] = reg->b.sim.signal;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
float regCalcPureDelay(struct reg_converter *reg, struct reg_meas_filter *meas_filter, uint32_t reg_period_iters)
/*---------------------------------------------------------------------------------------------------------*\
  This function will estimate the pure loop delay in regulation periods needed by regRstInit().
\*---------------------------------------------------------------------------------------------------------*/
{
    return((reg->sim_vs_pars.v_ref_delay_iters + reg->sim_vs_pars.step_rsp_time_iters +
            meas_filter->delay_iters[meas_filter->reg_select]) / (float)reg_period_iters);
}
/*---------------------------------------------------------------------------------------------------------*/
void regSetMode(struct reg_converter *reg, enum reg_mode reg_mode)
/*---------------------------------------------------------------------------------------------------------*\
  This function allows the regulation mode to be changed between voltage, current and field.
  This should be called at the start of an iteration before calling regConverter().
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t             idx;
    float                ref_offset;
    float                rate;
    struct reg_rst_vars *rst_vars = &reg->rst_vars;
    struct reg_rst_pars *rst_pars;

    // If regulation mode has changed

    if(reg_mode != reg->mode)
    {
        // If switching to VOLTAGE mode

        if(reg_mode == REG_VOLTAGE)
        {
            // Initialise voltage references according to previous regulation mode

            switch(reg->mode)
            {
                case REG_FIELD:

                    reg->v.ref = regRstAverageVref(&reg->rst_vars);
                    reg->v.ref_sat = reg->v.ref;
                    break;

                case REG_CURRENT:

                    reg->v.ref = regRstAverageVref(&reg->rst_vars);
                    reg->v.ref_sat = regLoadVrefSat(&reg->load_pars, reg->rst_vars.meas[0], reg->v.ref);
                    break;

                default:    // VOLTAGE or NONE

                    reg->v.ref_sat = reg->v.ref;
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
            regErrResetLimitsVars(&reg->i.err);
            regErrResetLimitsVars(&reg->b.err);
        }
        else // else switching to FIELD or CURRENT regulation
        {
            float              v_ref;
            struct reg_signal *r;

            // If closing loop on current, adjust v_ref for magnet saturation assuming current is invariant.
            // This assumes it is unlikely that the current regulation will start with the current ramping
            // fast while deep into the magnet saturation zone.

            r = reg->r       = (reg_mode == REG_FIELD ? &reg->b : &reg->i);
            rst_pars         = &r->rst_pars;
            v_ref            = (reg->mode == REG_CURRENT ? regLoadInverseVrefSat(&reg->load_pars, reg->i.meas.signal[REG_MEAS_UNFILTERED], reg->v.ref_limited) : reg->v.ref_limited);
            rate             = (reg->mode != REG_NONE    ? regMeasRate(&r->rate, rst_pars->period) : 0.0);
            reg->meas        = r->meas.signal[r->meas.reg_select];
            reg->ref_advance = rst_pars->track_delay_periods * rst_pars->period - r->meas.delay_iters[r->meas.reg_select] * reg->iter_period;

            rst_pars->ref_delay_periods = rst_pars->track_delay_periods +
                                         (r->meas.delay_iters[REG_MEAS_UNFILTERED] - r->meas.delay_iters[r->meas.reg_select]) / (float)rst_pars->period_iters;
            regErrResetLimitsVars(&r->err);

            // Prepare RST histories - assuming that v_ref has been constant when calculating rate

            reg->period_iters      = rst_pars->period_iters;
            reg->period            = rst_pars->period;
            reg->iteration_counter = reg->period_iters;

            ref_offset = rate * reg->ref_advance;

            for(idx = 0; idx < REG_N_RST_COEFFS; idx++)
            {
                rst_vars->act [idx] = v_ref;
                rst_vars->meas[idx] = reg->meas - rate * (float)(REG_N_RST_COEFFS - 1 - idx) * reg->period;
                rst_vars->ref [idx] = rst_vars->meas[idx] + ref_offset;
            }

            rst_vars->delayed_ref_index = 0;
            rst_vars->history_index     = idx;

            reg->ref = reg->ref_limited = reg->ref_rst = rst_vars->ref[idx - 1];
        }

        // Store the new regulation mode

        reg->mode = reg_mode;
    }

    // Reset max abs error whenever regSetMode is called, even if the mode doesn't change

    reg->i.err.max_abs_err = reg->b.err.max_abs_err = 0.0;
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t regConverter(struct reg_converter      *reg,                 // Regulation structure
                      float                     *ref,                 // Ref for voltage, current or field
                      float                      feedforward_v_ref,   // Feedforward voltage reference
                      uint32_t                   feedforward_control, // Feedforward enable/disable control
                      uint32_t                   max_abs_err_control) // Max abs error calc enable/disable
/*---------------------------------------------------------------------------------------------------------*\
  This function will control a converter in either open-loop voltage mode, or closed-loop current or
  field regulation modes.  It returns non-zero on iterations when regulation is active (i.e. always for
  VOLTAGE mode).
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t           reg_flag = 0;    // Returned flag indicating iterations when regulation is active
    struct reg_signal *r;               // Pointer to active regulation structure (reg->b or reg->i)

    // Calculate and check the voltage regulation limits

    regErrCheckLimits(&reg->v.err, 1, 1, reg->v.err.delayed_ref, reg->v.meas);

    // Check current measurement limits

    regLimMeas(&reg->i.lim_meas, reg->i.meas.signal[REG_MEAS_UNFILTERED]);

    // Check field measurement limits only when regulating field

    if(reg->mode == REG_FIELD)
    {
        regLimMeas(&reg->b.lim_meas, reg->b.meas.signal[REG_MEAS_UNFILTERED]);
    }

    // Calculate voltage reference limits for the measured current (V limits can depend on current)

    regLimVrefCalc(&reg->v.lim_ref, reg->i.meas.signal[REG_MEAS_UNFILTERED]);

    // Filter the field and current measurements and prepare to estimate measurement rate

    regMeasFilter(&reg->b.meas);
    regMeasRateStore(&reg->b.rate,reg->b.meas.signal[REG_MEAS_FILTERED],reg->b.rst_pars.period_iters);

    regMeasFilter(&reg->i.meas);
    regMeasRateStore(&reg->i.rate,reg->i.meas.signal[REG_MEAS_FILTERED],reg->i.rst_pars.period_iters);

    // If open-loop (voltage regulation) mode - apply voltage ref limits

    if(reg->mode == REG_VOLTAGE)
    {
        reg->v.ref = reg->v.ref_sat = *ref;              // Don't apply magnet saturation compensation

        reg->v.ref_limited = regLimRef(&reg->v.lim_ref, reg->iter_period, reg->v.ref, reg->v.ref_limited);

        reg->flags.ref_clip = reg->v.lim_ref.flags.clip;
        reg->flags.ref_rate = reg->v.lim_ref.flags.rate;

        *ref = reg->v.ref_limited;

        reg_flag = 1;
    }
    else  // else closed-loop on current or field
    {
        // Regulate current or field at the regulation period

        r = reg->r;

        if(reg->iteration_counter == 0)
        {
            float v_ref;
            float unfiltered_meas;

            reg->iteration_counter = reg->period_iters;

            reg_flag        = 1;
            reg->ref        = *ref;
            reg->meas       = r->meas.signal[r->meas.reg_select];
            unfiltered_meas = reg->i.meas.signal[REG_MEAS_UNFILTERED];

            if(feedforward_control == 0)
            {
                // Apply current reference clip and rate limits

                reg->ref_limited = regLimRef(&r->lim_ref, reg->period, reg->ref, reg->ref_limited);

                // Calculate voltage reference using RST algorithm

                reg->v.ref = regRstCalcAct(&r->rst_pars, &reg->rst_vars, reg->ref_limited, reg->meas);

                // Calculate magnet saturation compensation

                reg->v.ref_sat = (reg->mode == REG_CURRENT ? regLoadVrefSat(&reg->load_pars, unfiltered_meas, reg->v.ref) : reg->v.ref);

                // Apply voltage reference clip and rate limits

                reg->v.ref_limited = regLimRef(&reg->v.lim_ref, reg->period, reg->v.ref_sat, reg->v.ref_limited);

                // If voltage reference has been clipped

                if(reg->v.lim_ref.flags.clip || reg->v.lim_ref.flags.rate)
                {
                    // Back calculate the new v_ref before the saturation compensation

                    v_ref = (reg->mode == REG_CURRENT ? regLoadInverseVrefSat(&reg->load_pars, unfiltered_meas, reg->v.ref_limited) : reg->v.ref_limited);

                    // Back calculate new current reference to keep RST histories balanced

                    reg->ref_rst = regRstCalcRef(&r->rst_pars, &reg->rst_vars, v_ref, reg->meas);

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

                // Calculate v_ref with saturation compensation applied when regulating current

                reg->v.ref_sat = (reg->mode == REG_CURRENT ? regLoadVrefSat(&reg->load_pars, unfiltered_meas, feedforward_v_ref) : feedforward_v_ref);

                // Apply voltage reference limits

                reg->v.ref_limited = regLimRef(&reg->v.lim_ref, reg->period, reg->v.ref_sat, reg->v.ref_limited);

                // If v_ref was clipped then back calculate the new uncompensated v_ref

                if(reg->v.lim_ref.flags.clip || reg->v.lim_ref.flags.rate)
                {
                    v_ref = (reg->mode == REG_CURRENT ? regLoadInverseVrefSat(&reg->load_pars, unfiltered_meas, reg->v.ref_limited) : reg->v.ref_limited);
                    reg->flags.ref_rate = 1;
                }
                else
                {
                    v_ref = reg->v.ref;
                    reg->flags.ref_rate = 0;
                }

                // Back calculate the current reference that would produce this voltage reference

                reg->ref = reg->ref_limited = reg->ref_rst = regRstCalcRef(&r->rst_pars, &reg->rst_vars, v_ref, reg->meas);
            }

            regRstMeasTrackDelay(&reg->rst_vars, reg->period,r->lim_ref.rate_clip);

            regRstHistory(&reg->rst_vars);

            *ref = reg->ref_rst;
        }

        // Monitor regulation error using interpolation on the reference

        if(r->err_rate == REG_ERR_RATE_MEASUREMENT || reg_flag == 1)
        {
            reg->ref_delayed = regRstDelayedRef(&r->rst_pars, &reg->rst_vars);

            regErrCheckLimits(&r->err, !feedforward_control, max_abs_err_control,
                              reg->ref_delayed, r->meas.signal[REG_MEAS_UNFILTERED]);
        }

        reg->iteration_counter--;
    }

    return(reg_flag);
}
/*---------------------------------------------------------------------------------------------------------*/
void regSimulate(struct reg_converter *reg, float v_perturbation)
/*---------------------------------------------------------------------------------------------------------*\
  This function will simulate the voltage source and load and the measurements of the voltage, current
  and field. The voltage reference comes from reg->v.ref_limited which is calculated by calling
  regConverter().  A voltage perturbation can be included in the simulation via the v_perturbation parameter.
\*---------------------------------------------------------------------------------------------------------*/
{
    float v_circuit;      // Simulated v_circuit without V_REF_DELAY

    // Simulate voltage source response to v_ref without taking into account V_REF_DELAY

    v_circuit = regSimVs(&reg->sim_vs_pars, &reg->sim_vs_vars, reg->v.ref_limited);

    // Simulate load current and field in response to sim_advanced_v_circuit plus the perturbation

    regSimLoad(&reg->sim_load_pars, &reg->sim_load_vars, v_circuit + v_perturbation);

    // Use delays to estimate the measurement of the magnet's field and the circuit's current and voltage

    reg->b.sim.signal = regDelayCalc(&reg->b.sim.meas_delay, reg->sim_load_vars.magnet_field,
                                      reg->sim_load_pars.vs_undersampled_flag && reg->sim_load_pars.load_undersampled_flag);
    reg->i.sim.signal = regDelayCalc(&reg->i.sim.meas_delay, reg->sim_load_vars.circuit_current,
                                      reg->sim_load_pars.vs_undersampled_flag && reg->sim_load_pars.load_undersampled_flag);
    reg->v.sim.signal = regDelayCalc(&reg->v.sim.meas_delay, reg->sim_load_vars.circuit_voltage,
                                      reg->sim_load_pars.vs_undersampled_flag);

    // Store simulated voltage measurement without noise as the delayed ref for the v_err calculation

    reg->v.err.delayed_ref = reg->v.sim.signal;

    // Simulate noise and tone on simulated measurement of the magnet's field and the circuit's current and voltage

    reg->b.sim.signal += regMeasNoiseAndTone(&reg->b.sim.noise_and_tone);
    reg->i.sim.signal += regMeasNoiseAndTone(&reg->i.sim.noise_and_tone);
    reg->v.sim.signal += regMeasNoiseAndTone(&reg->v.sim.noise_and_tone);
}
// EOF

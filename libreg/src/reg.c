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
void regSetSimLoad(struct reg_converter *reg, struct reg_converter_pars *reg_pars, enum reg_mode reg_mode,
                   float sim_load_tc_error)
/*---------------------------------------------------------------------------------------------------------*\
  This function will initialise the simulated load structures with the specified load parameters
\*---------------------------------------------------------------------------------------------------------*/
{
    regSimLoadTcError(&reg_pars->sim_load_pars, &reg_pars->load_pars, sim_load_tc_error);

    regSimLoadInit(&reg_pars->sim_load_pars, reg->iter_period);

    switch(reg->mode = reg_mode)
    {
    default:

        regSimLoadSetVoltage(&reg_pars->sim_load_pars, &reg->sim_load_vars, reg->v_meas);
        break;

    case REG_CURRENT:

        regSimLoadSetCurrent(&reg_pars->sim_load_pars, &reg->sim_load_vars, reg->i_meas.meas[REG_MEAS_UNFILTERED]);
        break;

    case REG_FIELD:

        regSimLoadSetField(&reg_pars->sim_load_pars, &reg->sim_load_vars, reg->b_meas.meas[REG_MEAS_UNFILTERED]);
        break;
    }

    reg->v_meas = reg->sim_load_vars.voltage;
    
    reg->i_meas.meas[REG_MEAS_FILTERED] = reg->i_meas.meas[REG_MEAS_UNFILTERED] = reg->sim_load_vars.current;
    reg->b_meas.meas[REG_MEAS_FILTERED] = reg->b_meas.meas[REG_MEAS_UNFILTERED] = reg->sim_load_vars.field;
}
/*---------------------------------------------------------------------------------------------------------*/
void regSetMeas(struct reg_converter *reg, struct reg_converter_pars *reg_pars,
                float v_meas, float i_meas, float b_meas, uint32_t sim_meas_control)
/*---------------------------------------------------------------------------------------------------------*\
  This function will set the unfiltered measured values in the reg structure based on the sim_meas_control.
  When active, the measurements will be based on the voltage source and load simulation calculated by
  regSimulate().
\*---------------------------------------------------------------------------------------------------------*/
{
    if(sim_meas_control == 0)
    {
        // Use measured values for voltage, current and field

        reg->v_meas = v_meas;
        reg->i_meas.meas[REG_MEAS_UNFILTERED] = i_meas;
        reg->b_meas.meas[REG_MEAS_UNFILTERED] = b_meas;
    }
    else
    {
        // Use simulated measurements

        reg->v_meas = reg->v_sim.meas;
        reg->i_meas.meas[REG_MEAS_UNFILTERED] = reg->i_sim.meas;
        reg->b_meas.meas[REG_MEAS_UNFILTERED] = reg->b_sim.meas;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
float regCalcPureDelay(struct reg_converter *reg, struct reg_converter_pars *reg_pars,
                       struct reg_meas_filter *meas_filter, uint32_t reg_period_iters)
/*---------------------------------------------------------------------------------------------------------*\
  This function will estimate the pure loop delay in regulation periods needed by regRstInit().
\*---------------------------------------------------------------------------------------------------------*/
{
    return((reg_pars->sim_vs_pars.v_ref_delay_iters + reg_pars->sim_vs_pars.step_rsp_time_iters +
            meas_filter->meas_delay_iters[meas_filter->reg_select]) / (float)reg_period_iters);
}
/*---------------------------------------------------------------------------------------------------------*/
void regSetMode(struct reg_converter      *reg,
                struct reg_converter_pars *reg_pars,
                enum reg_mode              reg_mode)
/*---------------------------------------------------------------------------------------------------------*\
  This function allows the regulation mode to be changed between voltage, current and field.
  This should be called at the start of an iteration before calling regConverter().
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t             idx;
    float                v_ref;
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

                    reg->v_ref = regRstAverageVref(&reg->rst_vars);
                    reg->v_ref_sat = reg->v_ref;
                    break;

                case REG_CURRENT:

                    reg->v_ref = regRstAverageVref(&reg->rst_vars);
                    reg->v_ref_sat = regLoadVrefSat(&reg_pars->load_pars, reg->rst_vars.meas[0], reg->v_ref);
                    break;

                default:    // VOLTAGE or NONE

                    reg->v_ref_sat = reg->v_ref;
                    break;
            }

            reg->v_ref_limited = reg->v_ref_sat;

            // Calculate the ref advance for voltage mode

            reg->ref_advance = reg->iter_period *
                              (reg_pars->sim_vs_pars.v_ref_delay_iters + reg_pars->sim_vs_pars.step_rsp_time_iters);

            // Clear field and current regulation variables

            reg->ref         = 0.0;
            reg->ref_limited = 0.0;
            reg->ref_rst     = 0.0;
            reg->meas        = 0.0;
            reg->rst_vars.meas_track_delay_periods = 0.0;
            regErrResetLimitsVars(&reg->i_err);
            regErrResetLimitsVars(&reg->b_err);
        }
        else // else switching to FIELD or CURRENT regulation
        {
            // If closing loop on current, adjust v_ref for magnet saturation assuming current is invariant.
            // This assumes it is unlikely that the current regulation will start with the current ramping
            // fast while deep into the magnet saturation zone.

            if(reg_mode == REG_FIELD)
            {
                rst_pars         = &reg_pars->b_rst_pars;
                v_ref            = reg->v_ref_limited;
                rate             = regMeasRate(&reg->b_rate, rst_pars->period);
                reg->meas        = reg->b_meas.meas[reg->b_meas.reg_select];
                reg->ref_advance = reg_pars->b_rst_pars.track_delay_periods * rst_pars->period -
                                   reg->b_meas.meas_delay_iters[reg->b_meas.reg_select] * reg->iter_period;

                rst_pars->ref_delay_periods = reg_pars->b_rst_pars.track_delay_periods +
                                             (reg->b_meas.meas_delay_iters[REG_MEAS_UNFILTERED] -
                                              reg->b_meas.meas_delay_iters[reg->b_meas.reg_select]) /
                                              (float)(rst_pars->period_iters);
                regErrResetLimitsVars(&reg->i_err);
            }
            else
            {
                rst_pars         = &reg_pars->i_rst_pars;
                v_ref            = regLoadInverseVrefSat(&reg_pars->load_pars, reg->i_meas.meas[REG_MEAS_UNFILTERED], reg->v_ref_limited);
                rate             = regMeasRate(&reg->i_rate, rst_pars->period);
                reg->meas        = reg->i_meas.meas[reg->i_meas.reg_select];
                reg->ref_advance = reg_pars->i_rst_pars.track_delay_periods * rst_pars->period -
                                   reg->i_meas.meas_delay_iters[reg->i_meas.reg_select] * reg->iter_period;

                rst_pars->ref_delay_periods = reg_pars->i_rst_pars.track_delay_periods +
                                             (reg->i_meas.meas_delay_iters[REG_MEAS_UNFILTERED] -
                                              reg->i_meas.meas_delay_iters[reg->i_meas.reg_select]) /
                                              (float)(rst_pars->period_iters);
                regErrResetLimitsVars(&reg->b_err);
            }

            // Prepare RST histories - assuming that v_ref has been constant when calculating rate

            reg->period_iters      = rst_pars->period_iters;
            reg->period            = rst_pars->period;
            reg->iteration_counter = reg->period_iters;

            if(reg->mode == REG_NONE)
            {
                rate = 0;
            }

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

    reg->i_err.max_abs_err = reg->b_err.max_abs_err = 0.0;

}
/*---------------------------------------------------------------------------------------------------------*/
static void regField(struct reg_converter      *reg,                   // Regulation structure
                     struct reg_converter_pars *reg_pars,              // Regulation parameters structure
                     float                      feedforward_v_ref,     // Feedforward voltage reference
                     uint32_t                   feedforward_control)   // Feedforward enable/disable control
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from regConverter() when field regulation mode is enabled.  A field reference
  can be supplied in ref or a feedforward voltage reference can be supplied in feedforward_v_ref if
  feedforward_control is non-zero.  In this case the regulation algorithm is run backwards to calculate
  the field reference. Magnet saturation is a second order effect when regulating field so it is not
  compensated.
\*---------------------------------------------------------------------------------------------------------*/
{
    reg->meas = reg->b_meas.meas[reg->b_meas.reg_select];

    if(feedforward_control == 0)
    {
        // Apply field reference clip and rate limits

        reg->ref_limited = regLimRef(&reg->lim_b_ref, reg->period, reg->ref, reg->ref_limited);

        // Calculate voltage reference using RST algorithm (no magnet saturation compensation)

        reg->v_ref_sat = reg->v_ref = regRstCalcAct(&reg_pars->b_rst_pars, &reg->rst_vars,
                                                    reg->ref_limited, reg->meas);

        // Apply voltage reference clip and rate limits

        reg->v_ref_limited = regLimRef(&reg->lim_v_ref, reg->period, reg->v_ref, reg->v_ref_limited);

        // If voltage reference has been clipped

        if(reg->lim_v_ref.flags.clip || reg->lim_v_ref.flags.rate)
        {
            // Back calculate new current reference to keep RST histories balanced

            reg->ref_rst = regRstCalcRef(&reg_pars->b_rst_pars, &reg->rst_vars,
                                         reg->v_ref_limited, reg->meas);

            // Mark field reference as rate limited

            reg->lim_b_ref.flags.rate = 1;
        }
        else
        {
            reg->ref_rst = reg->ref_limited;
        }

        reg->flags.ref_clip = reg->lim_b_ref.flags.clip;
        reg->flags.ref_rate = reg->lim_b_ref.flags.rate;
    }
    else
    {   // Use openloop voltage reference to back calculate field reference

        // Apply voltage reference limits

        reg->v_ref_sat = reg->v_ref = feedforward_v_ref;

        reg->v_ref_limited = regLimRef(&reg->lim_v_ref, reg->period, feedforward_v_ref, reg->v_ref_limited);

        // Back calculate the current reference that would produce this voltage reference

        reg->ref = reg->ref_limited = reg->ref_rst =
            regRstCalcRef(&reg_pars->b_rst_pars, &reg->rst_vars, reg->v_ref_limited, reg->meas);

        // Set limit flags

        reg->flags.ref_clip = 0;
        reg->flags.ref_rate = (reg->lim_v_ref.flags.clip || reg->lim_v_ref.flags.rate);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void regCurrent(struct reg_converter      *reg,                   // Regulation structure
                       struct reg_converter_pars *reg_pars,              // Regulation parameters structure
                       float                      feedforward_v_ref,     // Feedforward voltage reference
                       uint32_t                   feedforward_control)   // Feedforward enable/disable control
/*---------------------------------------------------------------------------------------------------------*\
  This function is called from regConverter() when current regulation mode is enabled.  A current reference
  can be supplied in ref or a feedforward voltage reference can be supplied if feedforward_v_ref is
  feedforward_control is non-zero.  In this case the regulation algorithm is run backwards to calculate
  the current reference.  Unlike field regulation, this algorithm applies a compensation for the saturation
  of the magnet.
\*---------------------------------------------------------------------------------------------------------*/
{
    float v_ref;

    reg->meas = reg->i_meas.meas[reg->i_meas.reg_select];

    if(feedforward_control == 0)
    {
        // Apply current reference clip and rate limits

        reg->ref_limited = regLimRef(&reg->lim_i_ref, reg->period, reg->ref, reg->ref_limited);

        // Calculate voltage reference using RST algorithm

        reg->v_ref = regRstCalcAct(&reg_pars->i_rst_pars, &reg->rst_vars, reg->ref_limited, reg->meas);

        // Calculate magnet saturation compensation

        reg->v_ref_sat = regLoadVrefSat(&reg_pars->load_pars, reg->i_meas.meas[REG_MEAS_UNFILTERED], reg->v_ref);

        // Apply voltage reference clip and rate limits

        reg->v_ref_limited = regLimRef(&reg->lim_v_ref, reg->period, reg->v_ref_sat, reg->v_ref_limited);

        // If voltage reference has been clipped

        if(reg->lim_v_ref.flags.clip || reg->lim_v_ref.flags.rate)
        {
            // Back calculate the new v_ref before the saturation compensation

            v_ref = regLoadInverseVrefSat(&reg_pars->load_pars, reg->i_meas.meas[REG_MEAS_UNFILTERED], reg->v_ref_limited);

            // Back calculate new current reference to keep RST histories balanced

            reg->ref_rst = regRstCalcRef(&reg_pars->i_rst_pars, &reg->rst_vars, v_ref, reg->meas);

            // Mark current reference as rate limited

            reg->lim_i_ref.flags.rate = 1;
        }
        else
        {
            reg->ref_rst = reg->ref_limited;
        }

        reg->flags.ref_clip = reg->lim_i_ref.flags.clip;
        reg->flags.ref_rate = reg->lim_i_ref.flags.rate;
    }
    else
    {
      // Open-loop: Use feedforward_v_ref

        reg->flags.ref_clip = 0;
        reg->v_ref = feedforward_v_ref;

        // Calculate v_ref with saturation compensation applied

        reg->v_ref_sat = regLoadVrefSat(&reg_pars->load_pars, reg->i_meas.meas[REG_MEAS_UNFILTERED], feedforward_v_ref);

        // Apply voltage reference limits

        reg->v_ref_limited = regLimRef(&reg->lim_v_ref, reg->period, reg->v_ref_sat, reg->v_ref_limited);

        // If v_ref was clipped then back calculate the new uncompensated v_ref

        if(reg->lim_v_ref.flags.clip || reg->lim_v_ref.flags.rate)
        {
            v_ref = regLoadInverseVrefSat(&reg_pars->load_pars, reg->i_meas.meas[REG_MEAS_UNFILTERED], reg->v_ref_limited);
            reg->flags.ref_rate = 1;
        }
        else
        {
            v_ref = reg->v_ref;
            reg->flags.ref_rate = 0;
        }

        // Back calculate the current reference that would produce this voltage reference

        reg->ref = reg->ref_limited = reg->ref_rst =
            regRstCalcRef(&reg_pars->i_rst_pars, &reg->rst_vars, v_ref, reg->meas);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t regConverter(struct reg_converter      *reg,                 // Regulation structure
                      struct reg_converter_pars *reg_pars,            // Regulation parameters structure
                      float                     *ref,                 // Ref for voltage, current or field
                      float                      feedforward_v_ref,   // Feedforward voltage reference
                      uint32_t                   feedforward_control, // Feedforward enable/disable control
                      uint32_t                   max_abs_err_control) // Max abs error calc enable/disable
/*---------------------------------------------------------------------------------------------------------*\
  This function will control a converter in either open-loop voltage mode, or closed-loop current or
  field regulation modes.  It returns non-zero no iterations when regulation is active (i.e. always for
  VOLTAGE mode).
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t reg_flag = 0;      // Returned flag indicating iterations when regulation is active

    // Calculate and check the voltage regulation limits

    regErrCheckLimits(&reg->v_err, 1, 1, reg->v_err.delayed_ref, reg->v_meas);

    // Check current measurement limits

    regLimMeas(&reg->lim_i_meas, reg->i_meas.meas[REG_MEAS_UNFILTERED]);

    // Check field measurement limits only when regulating field

    if(reg->mode == REG_FIELD)
    {
        regLimMeas(&reg->lim_b_meas, reg->b_meas.meas[REG_MEAS_UNFILTERED]);
    }

    // Calculate voltage reference limits for the measured current (V limits can depend on current)

    regLimVrefCalc(&reg->lim_v_ref, reg->i_meas.meas[REG_MEAS_UNFILTERED]);

    // Filter the field and current measurements and prepare to estimate measurement rate

    regMeasFilter(&reg->b_meas);
    regMeasRateStore(&reg->b_rate,reg->b_meas.meas[REG_MEAS_FILTERED],reg_pars->b_rst_pars.period_iters);

    regMeasFilter(&reg->i_meas);
    regMeasRateStore(&reg->i_rate,reg->i_meas.meas[REG_MEAS_FILTERED],reg_pars->i_rst_pars.period_iters);

    // If open-loop (voltage regulation) mode - apply voltage ref limits

    if(reg->mode == REG_VOLTAGE)
    {
        reg->v_ref = reg->v_ref_sat = *ref;              // Don't apply magnet saturation compensation

        reg->v_ref_limited = regLimRef(&reg->lim_v_ref, reg->iter_period, reg->v_ref, reg->v_ref_limited);

        reg->flags.ref_clip = reg->lim_v_ref.flags.clip;
        reg->flags.ref_rate = reg->lim_v_ref.flags.rate;

        *ref = reg->v_ref_limited;

        reg_flag = 1;
    }
    else  // else closed-loop on current or field
    {
        // Regulate current or field at the regulation period

        if(reg->iteration_counter == 0)
        {
            reg_flag = 1;
            reg->ref = *ref;
            reg->iteration_counter = reg->period_iters;

            if(reg->mode == REG_FIELD)
            {
                regField(reg, reg_pars, feedforward_v_ref, feedforward_control);
                regRstMeasTrackDelay(&reg->rst_vars, reg->period,reg->lim_b_ref.rate_clip);
            }
            else
            {
                regCurrent(reg, reg_pars, feedforward_v_ref, feedforward_control);
                regRstMeasTrackDelay(&reg->rst_vars, reg->period,reg->lim_i_ref.rate_clip);
            }

            regRstHistory(&reg->rst_vars);

            *ref = reg->ref_rst;
        }

        // Monitor regulation error using interpolation on the reference

        if(reg->mode == REG_FIELD)
        {
            reg->ref_delayed = regRstDelayedRef(&reg_pars->b_rst_pars, &reg->rst_vars);

            regErrCheckLimits(&reg->b_err, !feedforward_control, max_abs_err_control,
                              reg->ref_delayed, reg->b_meas.meas[REG_MEAS_UNFILTERED]);

            reg->err         = reg->b_err.err;
            reg->max_abs_err = reg->b_err.max_abs_err;
        }
        else
        {
            reg->ref_delayed = regRstDelayedRef(&reg_pars->i_rst_pars, &reg->rst_vars);

            regErrCheckLimits(&reg->i_err, !feedforward_control, max_abs_err_control,
                              reg->ref_delayed, reg->i_meas.meas[REG_MEAS_UNFILTERED]);

            reg->err         = reg->i_err.err;
            reg->max_abs_err = reg->i_err.max_abs_err;
        }

        reg->iteration_counter--;
    }

    return(reg_flag);
}
/*---------------------------------------------------------------------------------------------------------*/
void regSimulate(struct reg_converter *reg, struct reg_converter_pars *reg_pars, float v_perturbation)
/*---------------------------------------------------------------------------------------------------------*\
  This function will simulate the voltage source and load and the measurements of the voltage, current
  and field. The voltage reference comes from reg->v_ref_limited which is calculated by calling
  regConverter().  A voltage perturbation can be included in the simulation via the v_perturbation parameter.
\*---------------------------------------------------------------------------------------------------------*/
{
    float sim_advanced_v_load;      // Simulated v_load without V_REF_DELAY

    // Simulate voltage source response to v_ref without taking into account V_REF_DELAY

    sim_advanced_v_load = regSimVs(&reg_pars->sim_vs_pars, &reg->sim_vs_vars, reg->v_ref_limited);

    // Simulate load current and field in response to sim_advanced_v_load plus the perturbation

    regSimLoad(&reg_pars->sim_load_pars, &reg->sim_load_vars, sim_advanced_v_load + v_perturbation);

    // Use delays to estimate the voltage across the load and the measurement of this voltage

    regDelayCalc(&reg->v_sim.delay, reg->sim_load_vars.voltage, &reg->v_sim.load, &reg->v_sim.meas);

    // Store simulated voltage measurement without noise as the delayed ref for the v_err calculation

    reg->v_err.delayed_ref = reg->v_sim.meas;

    // Use delays to estimate the current in the load and the measurement of this current

    regDelayCalc(&reg->i_sim.delay, reg->sim_load_vars.current, &reg->i_sim.load, &reg->i_sim.meas);

    // Use delays to estimate the field in the load and the measurement of this field

    regDelayCalc(&reg->b_sim.delay, reg->sim_load_vars.field, &reg->b_sim.load, &reg->b_sim.meas);

    // Simulate noise and tone on measurement of field, current and voltage

    reg->b_sim.meas += regMeasNoiseAndTone(&reg->b_sim.noise_and_tone);
    reg->i_sim.meas += regMeasNoiseAndTone(&reg->i_sim.noise_and_tone);
    reg->v_sim.meas += regMeasNoiseAndTone(&reg->v_sim.noise_and_tone);
}
// EOF


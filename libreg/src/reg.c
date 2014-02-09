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

#include "libreg.h"

/*---------------------------------------------------------------------------------------------------------*/
void regMeasFilterInit(struct reg_meas_pars *pars, struct reg_meas_vars *vars,
                      float  num[REG_N_IIR_COEFFS], float den[REG_N_IIR_COEFFS])
/*---------------------------------------------------------------------------------------------------------*\
  This function calculates the floating point correction and the order of the IIR filter. It also
  saves the filter coefficients in the filter parameter structure.
\*---------------------------------------------------------------------------------------------------------*/
{
    double           num0_correction = 0.0;
    uint32_t        i = REG_N_IIR_COEFFS;

    while(i-- > 0)
    {
        // Calculate the num[0] correction due to single precision floating point coefficients
        // For the gain of the filter to be unity, sum(num) must equal sum(den).

        num0_correction += (double)den[i] - num[i];

        pars->num[i] = num[i];
        pars->den[i] = den[i];

        // Analyse filter coefficients to derive the filter order and save filter coefficients.

        if(pars->order == 0 && (num[i] != 0.0 || den[i] != 0.0))
        {
            pars->order = i;
        }
    }

    pars->num0_correction = num0_correction;
}
/*---------------------------------------------------------------------------------------------------------*/
void regMeasFilterInitHistory(struct reg_meas_vars *vars, float init_meas)
/*---------------------------------------------------------------------------------------------------------*\
  This function calculates the floating point correction and the order of the IIR filter. It also initialises
  the history in case a simulation is being prepared with a non-zero initial measurement value.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t        i;

    // Initialise filter history

    for(i = 0; i < REG_N_IIR_COEFFS ; i++)
    {
        vars->iir_in[i] = vars->iir_out[i] = init_meas;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void regMeasFilter(struct reg_meas_pars *pars, struct reg_meas_vars *vars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will apply the IIR filter to the measurement.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    par_idx;
    uint32_t    var_idx  = vars->iir_latest_index;
    uint32_t    order = pars->order;
    float       filtered = 0.0;

    // Apply transfer function to IIR history

    for(par_idx = 1 ; par_idx <= order ; par_idx++)
    {
        filtered += pars->num[par_idx] * vars->iir_in[var_idx] - pars->den[par_idx] * vars->iir_out[var_idx];

        if(var_idx == 0)
        {
            var_idx = order;
        }
        else
        {
            var_idx--;
        }
    }

    vars->filtered = (filtered + vars->unfiltered * pars->num[0] + vars->unfiltered * pars->num0_correction) / pars->den[0];   // Normally den[0] will be 1.0

    // Adjust var_idx to next free record

    var_idx = ++vars->iir_latest_index;

    if(var_idx > order)
    {
        var_idx = vars->iir_latest_index = 0;
    }

    // Save new filter input and output in the history

    vars->iir_in [var_idx] = vars->unfiltered;
    vars->iir_out[var_idx] = vars->filtered;
}
/*---------------------------------------------------------------------------------------------------------*/
static void regDecimateMeas(struct reg_converter *reg, struct reg_meas_vars *meas, uint32_t decimate_flag)
/*---------------------------------------------------------------------------------------------------------*\
  This function will calculate the measurement for regulation based on the decimate flag.
  If disabled, the filtered measurement is used, otherwise, the filtered measurement is decimated (averaged)
  over the regulation period.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(decimate_flag)
    {
        meas->accumulator += meas->filtered;

        if(reg->iteration_counter == 0)
        {
            meas->regulated   = meas->accumulator / (float)reg->cl_period_iters;
            meas->accumulator = 0.0;
        }
    }
    else
    {
        meas->regulated   = meas->filtered;
        meas->accumulator = 0.0;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
float regCalcErrDelay(struct reg_rst_pars *rst_pars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will return the regulation error delay in iteration periods.  If decimation is active,
  it adds half a regulation period of delay.
\*---------------------------------------------------------------------------------------------------------*/
{
    float err_delay;

    err_delay = rst_pars->rst.track_delay;

    if(rst_pars->decimate_flag)
    {
        err_delay += 0.5 * rst_pars->period;
    }

    return(err_delay);
}
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

        regSimLoadSetVoltage(&reg_pars->sim_load_pars, &reg->sim_load_vars, reg->v_meas.unfiltered);
        break;

    case REG_CURRENT:

        regSimLoadSetCurrent(&reg_pars->sim_load_pars, &reg->sim_load_vars, reg->i_meas.unfiltered);
        break;

    case REG_FIELD:

        regSimLoadSetField(&reg_pars->sim_load_pars, &reg->sim_load_vars, reg->b_meas.unfiltered);
        break;
    }

    reg->v_meas.regulated = reg->v_meas.unfiltered = reg->sim_load_vars.voltage;
    reg->i_meas.regulated = reg->i_meas.unfiltered = reg->sim_load_vars.current;
    reg->b_meas.regulated = reg->b_meas.unfiltered = reg->sim_load_vars.field;
}
/*---------------------------------------------------------------------------------------------------------*/
void regSetMeasNoise(struct reg_converter *reg, float v_sim_noise, float i_sim_noise, float b_sim_noise)
/*---------------------------------------------------------------------------------------------------------*\
  This function will set the measured values in the reg structure based on the sim_meas_control.  When
  active, the measurements will be based on the voltage source and load simulation
\*---------------------------------------------------------------------------------------------------------*/
{
    reg->v_sim.noise = v_sim_noise;
    reg->i_sim.noise = i_sim_noise;
    reg->b_sim.noise = b_sim_noise;
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

        reg->v_meas.unfiltered = v_meas;
        reg->i_meas.unfiltered = i_meas;
        reg->b_meas.unfiltered = b_meas;
    }
    else
    {
        // Use simulated measurements

        reg->v_meas.unfiltered = reg->v_sim.meas;
        reg->i_meas.unfiltered = reg->i_sim.meas;
        reg->b_meas.unfiltered = reg->b_sim.meas;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void regSetVoltageMode(struct reg_converter *reg, struct reg_converter_pars *reg_pars)
/*---------------------------------------------------------------------------------------------------------*\
  This function switches the regulation mode to REG_VOLTAGE.  In this mode the reference function directly
  defines the voltage reference.  When switching from closed-loop regulation of current or field, the
  voltage reference is set to the average value found in the RST history.  Furthermore, if current was
  being regulated, then the v_ref in the RST history is adjusted for magnet saturation, which is not
  applied in voltage mode.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    idx;
    float       v_ref;

    if(reg->mode != REG_VOLTAGE)
    {
        // If field or current regulation was active

        if(reg->mode != REG_NONE)
        {
            // Calculate average v_ref from RST history

            v_ref = 0.0;

            for(idx = 0; idx < REG_N_RST_COEFFS ; idx++)
            {
                v_ref += reg->rst_vars.act[idx];
            }

            reg->v_ref = v_ref / REG_N_RST_COEFFS;

            // If regulating CURRENT then adjust for the magnet saturation

            if(reg->mode == REG_CURRENT)
            {
                reg->v_ref_sat = regLoadVrefSat(&reg_pars->load_pars, reg->rst_vars.meas[0], reg->v_ref);
            }
            else
            {
                reg->v_ref_sat = reg->v_ref;
            }

            reg->v_ref_limited = reg->v_ref_sat;
        }

        // Switch to voltage regulation mode

        reg->mode = REG_VOLTAGE;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void regSetMode(struct reg_converter      *reg,
                struct reg_converter_pars *reg_pars,
                enum reg_mode              mode,
                float                      meas,
                float                      rate)
/*---------------------------------------------------------------------------------------------------------*\
  This function allows the regulation mode to be changed between voltage, current and field.
  When switching to voltage regulation, only reg is actually needed and regSetVoltageMode() can be called
  directly if desired.  When switching to closed-loop on current or field then rest of the parameters
  must be supplied.  This should be called at the start of an iteration before calling regConverter().
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t             idx;
    float                v_ref;
    float                ref_offset;
    float                track_delay;
    struct reg_rst_pars *rst_pars;

    if(mode != reg->mode)
    {
        if(mode == REG_VOLTAGE)         // Open the loop and set v_ref to average of last few samples
        {
            regSetVoltageMode(reg, reg_pars);
        }
        else
        {
             // If closing loop on current, adjust v_ref for magnet saturation assuming current is invariant.
             // This assumes it is unlikely that the current regulation will start with the current ramping
             // fast while deep into the magnet saturation zone.

            if(mode == REG_FIELD)
            {
                rst_pars    = &reg_pars->b_rst_pars;
                v_ref       = reg->v_ref_limited;
                track_delay = rst_pars->rst.track_delay - reg->iter_period * (float)(rst_pars->period_iters + 1);

                if(rst_pars->decimate_flag)
                {
                    track_delay -= reg->iter_period * 0.5 * (float)(rst_pars->period_iters - 1);
                }

                regErrInitDelay(&reg->b_err, 0, track_delay, reg->iter_period);
            }
            else
            {
                rst_pars    = &reg_pars->i_rst_pars;
                v_ref       = regLoadInverseVrefSat(&reg_pars->load_pars, meas, reg->v_ref_limited);
                track_delay = rst_pars->rst.track_delay - reg->iter_period * (float)(rst_pars->period_iters + 1);

                if(rst_pars->decimate_flag)
                {
                    track_delay -= reg->iter_period * 0.5 * (float)(rst_pars->period_iters - 1);
                }

                regErrInitDelay(&reg->i_err, 0, track_delay, reg->iter_period);
            }

            // Prepare RST histories - assuming that v_ref has been constant when calculating rate

            reg->cl_period_iters   = rst_pars->period_iters;
            reg->cl_period         = rst_pars->period;
            reg->iteration_counter = reg->cl_period_iters - 1;
            ref_offset             = rate * regCalcErrDelay(rst_pars);

            for(idx = 0; idx < REG_N_RST_COEFFS; idx++)
            {
                reg->rst_vars.act [idx] = v_ref;
                reg->rst_vars.meas[idx] = meas - rate * (float)idx * reg->cl_period;
                reg->rst_vars.ref [idx] = reg->rst_vars.meas[idx] + ref_offset;
            }
            reg->ref = reg->ref_prev = reg->rst_vars.ref[0];

            reg->mode = mode;
        }
    }
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
    if(feedforward_control == 0)
    {
        // Apply field reference clip and rate limits

        reg->ref_limited = regLimRef(&reg->lim_b_ref, reg->cl_period, reg->ref, reg->ref_limited);

        // Calculate voltage reference using RST algorithm (no magnet saturation compensation)

        reg->v_ref_sat = reg->v_ref = regRstCalcAct(&reg_pars->b_rst_pars, &reg->rst_vars,
                                                    reg->ref_limited, reg->b_meas.regulated);

        // Apply voltage reference clip and rate limits

        reg->v_ref_limited = regLimRef(&reg->lim_v_ref, reg->cl_period, reg->v_ref, reg->v_ref_limited);

        // If voltage reference has been clipped

        if(reg->lim_v_ref.flags.clip || reg->lim_v_ref.flags.rate)
        {
            // Back calculate new current reference to keep RST histories balanced

            reg->ref_rst = regRstCalcRef(&reg_pars->b_rst_pars, &reg->rst_vars,
                                         reg->v_ref_limited, reg->b_meas.regulated);

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

        reg->v_ref_limited = regLimRef(&reg->lim_v_ref, reg->cl_period, feedforward_v_ref, reg->v_ref_limited);

        // Back calculate the current reference that would produce this voltage reference

        reg->ref = reg->ref_limited = reg->ref_rst =
            regRstCalcRef(&reg_pars->b_rst_pars, &reg->rst_vars, reg->v_ref_limited, reg->b_meas.regulated);

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

    if(feedforward_control == 0)
    {
        // Apply current reference clip and rate limits

        reg->ref_limited = regLimRef(&reg->lim_i_ref, reg->cl_period, reg->ref, reg->ref_limited);

        // Calculate voltage reference using RST algorithm

        reg->v_ref = regRstCalcAct(&reg_pars->i_rst_pars, &reg->rst_vars, reg->ref_limited, reg->i_meas.regulated);

        // Calculate magnet saturation compensation

        reg->v_ref_sat = regLoadVrefSat(&reg_pars->load_pars, reg->i_meas.unfiltered, reg->v_ref);

        // Apply voltage reference clip and rate limits

        reg->v_ref_limited = regLimRef(&reg->lim_v_ref, reg->cl_period, reg->v_ref_sat, reg->v_ref_limited);

        // If voltage reference has been clipped

        if(reg->lim_v_ref.flags.clip || reg->lim_v_ref.flags.rate)
        {
            // Back calculate the new v_ref before the saturation compensation

            v_ref = regLoadInverseVrefSat(&reg_pars->load_pars, reg->i_meas.regulated, reg->v_ref_limited);

            // Back calculate new current reference to keep RST histories balanced

            reg->ref_rst = regRstCalcRef(&reg_pars->i_rst_pars, &reg->rst_vars, v_ref, reg->i_meas.regulated);

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

        reg->v_ref_sat = regLoadVrefSat(&reg_pars->load_pars, reg->i_meas.unfiltered, feedforward_v_ref);

        // Apply voltage reference limits

        reg->v_ref_limited = regLimRef(&reg->lim_v_ref, reg->cl_period, reg->v_ref_sat, reg->v_ref_limited);

        // If v_ref was clipped then back calculate the new uncompensated v_ref

        if(reg->lim_v_ref.flags.clip || reg->lim_v_ref.flags.rate)
        {
            v_ref = regLoadInverseVrefSat(&reg_pars->load_pars, reg->i_meas.unfiltered, reg->v_ref_limited);
            reg->flags.ref_rate = 1;
        }
        else
        {
            v_ref = reg->v_ref;
            reg->flags.ref_rate = 0;
        }

        // Back calculate the current reference that would produce this voltage reference

        reg->ref = reg->ref_limited = reg->ref_rst =
            regRstCalcRef(&reg_pars->i_rst_pars, &reg->rst_vars, v_ref, reg->i_meas.regulated);
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

    regErrCheckLimits(&reg->v_err, 1, 1, reg->v_err.delayed_ref, reg->v_meas.unfiltered);

    // Check current measurement limits

    regLimMeas(&reg->lim_i_meas, reg->i_meas.unfiltered);

    // Check field measurment limits only when regulating field

    if(reg->mode == REG_FIELD)
    {
        regLimMeas(&reg->lim_b_meas, reg->b_meas.unfiltered);
    }

    // Calculate voltage reference limits for the measured current (V limits can depend on current)

    regLimVrefCalc(&reg->lim_v_ref, reg->i_meas.unfiltered);

    // Filter the measurements

    regMeasFilter(&reg_pars->v_meas, &reg->v_meas);
    regMeasFilter(&reg_pars->i_meas, &reg->i_meas);
    regMeasFilter(&reg_pars->b_meas, &reg->b_meas);

    // Decimate the filtered measurements if required

    regDecimateMeas(reg, &reg->v_meas, 0);
    regDecimateMeas(reg, &reg->i_meas, (reg->mode == REG_CURRENT && reg_pars->i_rst_pars.decimate_flag));
    regDecimateMeas(reg, &reg->b_meas, (reg->mode == REG_FIELD   && reg_pars->b_rst_pars.decimate_flag));

    // If open-loop (voltage regulation) mode - apply voltage ref limits

    if(reg->mode == REG_VOLTAGE)
    {
        reg->v_ref = reg->v_ref_sat = *ref;              // Don't apply magnet saturation compensation

        reg->v_ref_limited = regLimRef(&reg->lim_v_ref, reg->iter_period, reg->v_ref, reg->v_ref_limited);

        reg->flags.ref_clip = reg->lim_v_ref.flags.clip;
        reg->flags.ref_rate = reg->lim_v_ref.flags.rate;

        *ref = reg->v_ref_limited;

        // Clear current/field regulation error

        if(reg->i_err.limits.err > 0.0)
        {
            reg->i_err.limits.err          = 0.0;
            reg->i_err.limits.warning.flag = 0;
            reg->i_err.limits.fault.flag   = 0;
        }    

        if(reg->b_err.limits.err > 0.0)
        {
            reg->b_err.limits.err          = 0.0;
            reg->b_err.limits.warning.flag = 0;
            reg->b_err.limits.fault.flag   = 0;
        }    

        reg_flag = 1;
    }
    else  // else closed-loop on current or field
    {
        // Regulate current or field at the regulation period

        if(reg->iteration_counter == 0)
        {
            reg_flag = 1;
            reg->ref = *ref;
            reg->iteration_counter = reg->cl_period_iters;

            if(reg->mode == REG_FIELD)
            {
                regField(reg, reg_pars, feedforward_v_ref, feedforward_control);
            }
            else
            {
                regCurrent(reg, reg_pars, feedforward_v_ref, feedforward_control);
            }

            regRstHistory(&reg->rst_vars);

            reg->ref_rate = (reg->ref_limited - reg->ref_prev) / (float)reg->cl_period_iters;
            reg->ref_prev =  reg->ref_limited;

            *ref = reg->ref_rst;
        }

        // Monitor regulation error using interpolation on the reference

        reg->ref_interpolated = reg->ref_limited - reg->ref_rate * (float)(reg->iteration_counter);

        if(reg->mode == REG_CURRENT)
        {
            regErrCalc(&reg->i_err, !feedforward_control, max_abs_err_control,
                        reg->ref_interpolated, reg->i_meas.unfiltered);

            reg->err         = reg->i_err.limits.err;
            reg->max_abs_err = reg->i_err.limits.max_abs_err;
        }
        else
        {
            regErrCalc(&reg->b_err, !feedforward_control, max_abs_err_control,
                        reg->ref_interpolated, reg->b_meas.unfiltered);

            reg->err         = reg->b_err.limits.err;
            reg->max_abs_err = reg->b_err.limits.max_abs_err;
        }

        reg->iteration_counter--;
    }


    return(reg_flag);
}
/*---------------------------------------------------------------------------------------------------------*/
void regSimulate(struct reg_converter *reg, struct reg_converter_pars *reg_pars, float v_perturbation)
/*---------------------------------------------------------------------------------------------------------*\
  This function will simulate the voltage source and load and the measurements of the voltage, current
  and field. The voltage reference comes from reg->v_ref_limited which is calcualted by calling
  regConverter().  A voltage perturbation can be included in the simulation via the v_perturbation parameter.
\*---------------------------------------------------------------------------------------------------------*/
{
    float sim_v_load;

    // Simulate voltage source response to v_ref

    sim_v_load = regSimVs(&reg_pars->sim_vs_pars, &reg->sim_vs_vars, reg->v_ref_limited);

    // Simulate load current and field in response to sim_v_load plus the perturbation

    regSimLoad(&reg_pars->sim_load_pars, &reg->sim_load_vars, sim_v_load + v_perturbation);

    // Simulate voltage measurements using appropriate delay

    regDelayCalc(&reg->v_sim.delay, reg->sim_load_vars.voltage, &reg->v_sim.meas);

    // Store simulated voltage measurement without noise as the delayed ref for the v_err calculation

    reg->v_err.delayed_ref = reg->v_sim.meas;

    // Simulate noise on voltage measurement

    if(reg->v_sim.noise > 0.0)
    {
        reg->v_sim.meas += regSimNoise(reg->v_sim.noise);
    }

    // Apply delay and noise to simulated current measurement

    regDelayCalc(&reg->i_sim.delay, reg->sim_load_vars.current, &reg->i_sim.meas);

    if(reg->i_sim.noise > 0.0)
    {
        reg->i_sim.meas += regSimNoise(reg->i_sim.noise);
    }

    // Apply delay and noise to simulated field measurement

    regDelayCalc(&reg->b_sim.delay, reg->sim_load_vars.field, &reg->b_sim.meas);

    if(reg->b_sim.noise > 0.0)
    {
        reg->b_sim.meas += regSimNoise(reg->b_sim.noise);
    }
}
// EOF


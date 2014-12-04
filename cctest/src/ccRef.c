/*---------------------------------------------------------------------------------------------------------*\
  File:     ccRef.c                                                                     Copyright CERN 2014

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

  Purpose:  Function generation library test functions

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include cctest program header files

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRun.h"
#include "ccRef.h"

/*---------------------------------------------------------------------------------------------------------*/
static enum fg_limits_polarity ccRefLimitsPolarity(uint32_t invert_limits, uint32_t pol_swi_auto)
/*---------------------------------------------------------------------------------------------------------*/
{
    if(pol_swi_auto == REG_ENABLED)
    {
        return(FG_LIMITS_POL_AUTO);        // Limits should be tested based upon the polarity of the function
    }
    else if(invert_limits == REG_ENABLED)
    {
        return(FG_LIMITS_POL_NEGATIVE);    // Limits should be inverted
    }

    return(FG_LIMITS_POL_NORMAL);          // Normal limits with no manipulation
}
/*---------------------------------------------------------------------------------------------------------*/
bool ccRefDirectGen(struct fg_table_pars *pars, const double *time, float *ref)
/*---------------------------------------------------------------------------------------------------------*/
{
    double                func_time;              // Time since end of run delay
    float                 prev_rate;
    static float          prev_ref;
    static float          next_ref;
    static bool           ref_running;
    static struct fg_ramp_config config;

    // Coast during run delay

    func_time = *time - pars->delay;

    if(func_time <= 0.0)
    {
        return(true);
    }

    // If DIRECT function is already running

    if(pars->seg_idx > 0)
    {
        prev_rate = (*ref - prev_ref) / conv.reg_period;
        prev_ref  = *ref;
    }
    else // DIRECT has not yet started
    {
        // Prepare to force initialisation of first RAMP function

        ref_running   = false;
        prev_rate     = 0.0;
        prev_ref      = *ref;
        config.final  = 1.0E30;
    }

    // Scan through table to find segment containing the current time

    while(func_time >= pars->time[pars->seg_idx])      // while time exceeds end of segment
    {
        // If vector is already complete or is now complete

        if(pars->seg_idx >= pars->n_elements || ++pars->seg_idx >= pars->n_elements)
        {
            // Return function running flag from RAMP function

            return(ref_running);
        }
    }

    // If target reference has changed then re-arm the RAMP function

    if(pars->ref[pars->seg_idx] != next_ref)
    {
        // Initialise RAMP config based on reg_mode

        config.acceleration = ccpars_default.pars[conv.reg_mode].acceleration;
        config.deceleration = ccpars_default.pars[conv.reg_mode].deceleration;
        config.linear_rate  = ccpars_default.pars[conv.reg_mode].linear_rate;

        config.final = next_ref = pars->ref[pars->seg_idx];

        // Clip reference

        if(conv.reg_signal->lim_ref.invert_limits == REG_DISABLED)
        {
            if(next_ref > conv.lim_ref->pos)
            {
                config.final = conv.lim_ref->pos;
            }
            else if(next_ref < conv.lim_ref->neg)
            {
                config.final = conv.lim_ref->neg;
            }
        }
        else
        {
            if(next_ref > -conv.lim_ref->neg)
            {
                config.final = -conv.lim_ref->neg;
            }
            else if(next_ref < -conv.lim_ref->pos)
            {
                config.final = -conv.lim_ref->pos;
            }
        }

        // Initialise RAMP to new reference

        fgRampCalc(&config,
                   func_time - conv.reg_period,
                   *ref,
                   prev_rate,
                   &ccrun.prefunc.pars,
                   NULL);
    }

    ref_running = fgRampGen(&ccrun.prefunc.pars, &func_time, ref);

    return(true);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitPLEP(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_plep_config config;

    // Prepare PLEP config structure

    config.final        = ccpars_plep.final       [cyc_sel][0];
    config.acceleration = ccpars_plep.acceleration[cyc_sel][0];
    config.linear_rate  = ccpars_plep.linear_rate [cyc_sel][0];
    config.final_rate   = ccpars_plep.final_rate  [cyc_sel][0];
    config.exp_tc       = ccpars_plep.exp_tc      [cyc_sel][0];
    config.exp_final    = ccpars_plep.exp_final   [cyc_sel][0];

    // Try to initialise the PLEP

    if(fgPlepInit(  ccrun.fg_limits,
                    ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                    &config,
                    ccpars_global.run_delay,
                    ccpars_plep.initial_ref[cyc_sel][0],
                    &ccpars_plep.pars[cyc_sel],
                    fg_meta) != FG_OK)
    {
        ccTestPrintError("failed to initialise PLEP(%u) segment %u : %s : %g,%g,%g,%g", cyc_sel,
                fg_meta->error.index,
                ccParsEnumString(enum_fg_error, fg_meta->fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitRAMP(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_ramp_config config;

    // Prepare RAMP config structure

    config.final        = ccpars_ramp.final       [cyc_sel][0];
    config.acceleration = ccpars_ramp.acceleration[cyc_sel][0];
    config.linear_rate  = ccpars_ramp.linear_rate [cyc_sel][0];
    config.deceleration = ccpars_ramp.deceleration[cyc_sel][0];

    // If initial rate is zero, use normal initialisation with limits checking

    if(fgRampInit(  ccrun.fg_limits,
                    ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                    &config,
                    ccpars_global.run_delay,
                    ccpars_ramp.initial_ref[cyc_sel][0],
                    &ccpars_ramp.pars[cyc_sel],
                    fg_meta) != FG_OK)
    {
        ccTestPrintError("failed to initialise RAMP (%u) segment %u : %s : %g,%g,%g,%g", cyc_sel,
                fg_meta->error.index,
                ccParsEnumString(enum_fg_error, fg_meta->fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitPPPL(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_pppl_config config;

    // Set pppl config structure

    memcpy(&config.acceleration1, &ccpars_pppl.acceleration1[cyc_sel], FG_MAX_PPPLS * sizeof(float));
    memcpy(&config.acceleration2, &ccpars_pppl.acceleration2[cyc_sel], FG_MAX_PPPLS * sizeof(float));
    memcpy(&config.acceleration3, &ccpars_pppl.acceleration3[cyc_sel], FG_MAX_PPPLS * sizeof(float));
    memcpy(&config.rate2        , &ccpars_pppl.rate2        [cyc_sel], FG_MAX_PPPLS * sizeof(float));
    memcpy(&config.rate4        , &ccpars_pppl.rate4        [cyc_sel], FG_MAX_PPPLS * sizeof(float));
    memcpy(&config.ref4         , &ccpars_pppl.ref4         [cyc_sel], FG_MAX_PPPLS * sizeof(float));
    memcpy(&config.duration4    , &ccpars_pppl.duration4    [cyc_sel], FG_MAX_PPPLS * sizeof(float));

    config.numels_acceleration1 = pppl_pars[1].num_elements[cyc_sel];
    config.numels_acceleration2 = pppl_pars[2].num_elements[cyc_sel];
    config.numels_acceleration3 = pppl_pars[3].num_elements[cyc_sel];
    config.numels_rate2         = pppl_pars[4].num_elements[cyc_sel];
    config.numels_rate4         = pppl_pars[5].num_elements[cyc_sel];
    config.numels_ref4          = pppl_pars[6].num_elements[cyc_sel];
    config.numels_duration4     = pppl_pars[7].num_elements[cyc_sel];

    // Try to initialise the PPPL

    if(fgPpplInit(  ccrun.fg_limits,
                    ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                    &config,
                    ccpars_global.run_delay,
                    ccpars_pppl.initial_ref[cyc_sel][0],
                    &ccpars_pppl.pars[cyc_sel],
                    fg_meta) != FG_OK)
    {
        ccTestPrintError("failed to initialise PPPL(%u) error_index %u : %s : %g,%g,%g,%g", cyc_sel,
                fg_meta->error.index,
                ccParsEnumString(enum_fg_error, fg_meta->fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitTABLE(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_table_config config;                 // Libfg config struct for TABLE

    // Link table data to table config structure

    config.ref             = ccpars_table.ref[cyc_sel];
    config.time            = ccpars_table.time[cyc_sel];
    config.ref_n_elements  = table_pars[0].num_elements[cyc_sel];
    config.time_n_elements = table_pars[1].num_elements[cyc_sel];

    // Try to initialise the TABLE

    if(fgTableInit( ccrun.fg_limits,
                    ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                    &config,
                    ccpars_global.run_delay,
                    conv.iter_period,
                    &ccpars_table.pars[cyc_sel],
                    fg_meta) != FG_OK)
    {
        ccTestPrintError("failed to initialise TABLE(%u) point %u : %s : %g,%g,%g,%g", cyc_sel,
                fg_meta->error.index,
                ccParsEnumString(enum_fg_error, fg_meta->fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitSTEPS(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_test_config config;                 // Libfg config struct for TEST

    // Prepare TEST config structure

    config.type         = FG_TEST_STEPS;
    config.amplitude_pp = ccpars_test.amplitude_pp[cyc_sel][0];
    config.num_cycles   = ccpars_test.num_cycles  [cyc_sel][0];
    config.period       = ccpars_test.period      [cyc_sel][0];

    // Try to initialise the STEPS

    if(fgTestInit(  ccrun.fg_limits,
                    ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                    &config,
                    ccpars_global.run_delay,
                    ccpars_test.initial_ref[cyc_sel][0],
                    &ccpars_test.pars[cyc_sel],
                    fg_meta) != FG_OK)
    {
        ccTestPrintError("failed to initialise STEPS(%u) : %s : %g,%g,%g,%g", cyc_sel,
                ccParsEnumString(enum_fg_error, fg_meta->fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitSQUARE(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_test_config config;                 // Libfg config struct for TEST

    // Prepare TEST config structure

    config.type         = FG_TEST_SQUARE;
    config.amplitude_pp = ccpars_test.amplitude_pp[cyc_sel][0];
    config.num_cycles   = ccpars_test.num_cycles  [cyc_sel][0];
    config.period       = ccpars_test.period      [cyc_sel][0];

    // Try to initialise the SQUARE

    if(fgTestInit(  ccrun.fg_limits,
                    ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                    &config,
                    ccpars_global.run_delay,
                    ccpars_test.initial_ref[cyc_sel][0],
                    &ccpars_test.pars[cyc_sel],
                    fg_meta) != FG_OK)
    {
        ccTestPrintError("failed to initialise SQUARE(%u) : %s : %g,%g,%g,%g", cyc_sel,
                ccParsEnumString(enum_fg_error, fg_meta->fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitSINE(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_test_config config;                 // Libfg config struct for TEST

    // Prepare TEST config structure

    config.type         = FG_TEST_SINE;
    config.amplitude_pp = ccpars_test.amplitude_pp[cyc_sel][0];
    config.num_cycles   = ccpars_test.num_cycles  [cyc_sel][0];
    config.period       = ccpars_test.period      [cyc_sel][0];
    config.use_window   = ccpars_test.use_window  [cyc_sel][0];

    // Try to initialise the SINE

    if(fgTestInit(  ccrun.fg_limits,
                    ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                    &config,
                    ccpars_global.run_delay,
                    ccpars_test.initial_ref[cyc_sel][0],
                    &ccpars_test.pars[cyc_sel],
                    fg_meta) != FG_OK)
    {
        ccTestPrintError("failed to initialise SINE(%u) : %s : %g,%g,%g,%g", cyc_sel,
                ccParsEnumString(enum_fg_error, fg_meta->fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitCOSINE(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_test_config config;                 // Libfg config struct for TEST

    // Prepare TEST config structure

    config.type         = FG_TEST_COSINE;
    config.amplitude_pp = ccpars_test.amplitude_pp[cyc_sel][0];
    config.num_cycles   = ccpars_test.num_cycles  [cyc_sel][0];
    config.period       = ccpars_test.period      [cyc_sel][0];
    config.use_window   = ccpars_test.use_window  [cyc_sel][0];

    // Try to initialise the COSINE

    if(fgTestInit(  ccrun.fg_limits,
                    ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                    &config,
                    ccpars_global.run_delay,
                    ccpars_test.initial_ref[cyc_sel][0],
                    &ccpars_test.pars[cyc_sel],
                    fg_meta) != FG_OK)
    {
        ccTestPrintError("failed to initialise COSINE(%u) : %s : %g,%g,%g,%g", cyc_sel,
                ccParsEnumString(enum_fg_error, fg_meta->fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitLTRIM(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_trim_config config;

    // Prepare TRIM config structure

    config.type     = FG_TRIM_LINEAR;
    config.final    = ccpars_trim.final   [cyc_sel][0];
    config.duration = ccpars_trim.duration[cyc_sel][0];

    // Try to initialise the LTRIM

    if(fgTrimInit( ccrun.fg_limits,
                    ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                    &config,
                    ccpars_global.run_delay,
                    ccpars_trim.initial_ref[cyc_sel][0],
                    &ccpars_trim.pars[cyc_sel],
                    fg_meta) != FG_OK)
    {
        ccTestPrintError("failed to initialise LTRIM(%u) : %s : %g,%g,%g,%g", cyc_sel,
                ccParsEnumString(enum_fg_error, fg_meta->fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitCTRIM(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_trim_config config;

    // Prepare TRIM config structure

    config.type     = FG_TRIM_CUBIC;
    config.final    = ccpars_trim.final   [cyc_sel][0];
    config.duration = ccpars_trim.duration[cyc_sel][0];

    // Try to initialise the CTRIM

    if(fgTrimInit(  ccrun.fg_limits,
                    ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                    &config,
                    ccpars_global.run_delay,
                    ccpars_trim.initial_ref[cyc_sel][0],
                    &ccpars_trim.pars[cyc_sel],
                    fg_meta) != FG_OK)
    {
        ccTestPrintError("failed to initialise CTRIM(%u) : %s : %g,%g,%g,%g", cyc_sel,
                ccParsEnumString(enum_fg_error, fg_meta->fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitPULSE(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_trim_config config;

    // Prepare TRIM config structure to create a flat reference of the required duration

    config.type     = FG_TRIM_LINEAR;
    config.final    = ccpars_pulse.ref     [cyc_sel][0];
    config.duration = ccpars_pulse.duration[cyc_sel][0];

    // Try to initialise the LTRIM to produce the PULSE

    if(fgTrimInit(  ccrun.fg_limits,
                    ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                    &config,
                    ccpars_global.run_delay + ccpars_pulse.time[cyc_sel][0],
                    config.final,
                    &ccpars_pulse.pars[cyc_sel],
                    fg_meta) != FG_OK)
    {
        ccTestPrintError("failed to initialise PULSE(%u) : %s : %g,%g,%g,%g", cyc_sel,
                ccParsEnumString(enum_fg_error, fg_meta->fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
// EOF


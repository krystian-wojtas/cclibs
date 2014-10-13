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
static char * ccRefErrMsg(enum fg_error fg_error)
/*---------------------------------------------------------------------------------------------------------*\
  This uses a switch to be safe in case enum fg_errno is modified and the numbers change.
\*---------------------------------------------------------------------------------------------------------*/
{
    switch(fg_error)
    {
    case FG_OK:                         return("ok");
    case FG_BAD_ARRAY_LEN:              return("bad array len");
    case FG_BAD_PARAMETER:              return("bad parameter");
    case FG_INVALID_TIME:               return("invalid time");
    case FG_OUT_OF_ACCELERATION_LIMITS: return("out of acceleration limits");
    case FG_OUT_OF_LIMITS:              return("out of limits");
    case FG_OUT_OF_RATE_LIMITS:         return("out of rate limits");
    }

    return("unknown error");
}
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
    double          func_time;              // Time since end of run delay
    float           prev_rate;
    static float    prev_ref;
    static bool     ref_running;

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
        ccpars_prefunc.config.final = 1.0E30;
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

    if(pars->ref[pars->seg_idx] != ccpars_prefunc.config.final)
    {
        struct fg_meta fg_meta;

        ccpars_prefunc.config.final = pars->ref[pars->seg_idx];

        fgRampCalc(&ccpars_prefunc.config,
                   func_time - conv.reg_period,
                   *ref,
                   prev_rate,
                   &ccpars_prefunc.pars,
                   &fg_meta);
    }

    ref_running = fgRampGen(&ccpars_prefunc.pars, &func_time, ref);

    return(true);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitSTART(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Initialise a RAMP from zero to the minimum level, unless this is zero

    if(ccrun.fg_limits == NULL)
    {
        ccTestPrintError("START function requires GLOBAL FG_LIMITS to be ENABLED");
        return(EXIT_FAILURE);
    }

    ccpars_start.config.final = ccrun.fg_limits->min * (ccpars_limits.invert == REG_ENABLED ? -1.0 : 1.0);

    if(ccpars_start.config.final == 0.0)
    {
        ccTestPrintError("START function requires non-zero min limit to ramp to");
        return(EXIT_FAILURE);
    }

    fg_error = fgRampInit(NULL, FG_LIMITS_POL_NORMAL,
                          &ccpars_start.config,
                          ccpars_global.run_delay,    // delay
                          0.0,                        // initial ref
                          &ccpars_start.pars,
                          fg_meta);

    // Report error if initialisation fails - it can only be BAD_PARAMETER

    if(fg_error != FG_OK)
    {
        ccTestPrintError("START function requires non-zero ACCELERATION, DECELERATION");
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitPLEP(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Try to initialise the PLEP

    fg_error = fgPlepInit(ccrun.fg_limits,
                          ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                          &ccpars_plep.config,
                          ccpars_global.run_delay, ccpars_plep.initial_ref,
                          &ccpars_plep.pars, fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        ccTestPrintError("failed to initialise PLEP (segment %u) : %s : %g,%g,%g,%g",
                fg_meta->error.index,ccRefErrMsg(fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitRAMP(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error = FG_OK;

    // Try to initialise the RAMP according to the initial rate parameter

    if(ccpars_ramp.initial_rate != 0.0)
    {
        // If initial rate isn't zero then initialise with no limits checking

        if(ccpars_ramp.config.acceleration == 0.0 || ccpars_ramp.config.deceleration == 0.0)
        {
            fgResetMeta(fg_meta, NULL, 0.0);
            fg_error = FG_BAD_PARAMETER;
        }
        else
        {
            fgResetMeta(fg_meta, NULL, ccpars_ramp.initial_ref);

            fgRampCalc(&ccpars_ramp.config,
                        ccpars_global.run_delay,
                        ccpars_ramp.initial_ref,
                        ccpars_ramp.initial_rate,
                        &ccpars_ramp.pars,
                        fg_meta);
        }
    }
    else
    {
        // If initial rate is zero, use normal initialisation with limits checking

        fg_error = fgRampInit(ccrun.fg_limits,
                              ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                              &ccpars_ramp.config,
                              ccpars_global.run_delay,
                              ccpars_ramp.initial_ref,
                              &ccpars_ramp.pars,
                              fg_meta);
    }

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        ccTestPrintError("failed to initialise RAMP (segment %u) : %s : %g,%g,%g,%g",
                fg_meta->error.index,ccRefErrMsg(fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitPPPL(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Set up pppl array lengths

    ccpars_pppl.config.numels_acceleration1 = pppl_pars[1].num_elements;
    ccpars_pppl.config.numels_acceleration2 = pppl_pars[2].num_elements;
    ccpars_pppl.config.numels_acceleration3 = pppl_pars[3].num_elements;
    ccpars_pppl.config.numels_rate2         = pppl_pars[4].num_elements;
    ccpars_pppl.config.numels_rate4         = pppl_pars[5].num_elements;
    ccpars_pppl.config.numels_ref4          = pppl_pars[6].num_elements;
    ccpars_pppl.config.numels_duration4     = pppl_pars[7].num_elements;

    // Try to initialise the PPPL

    fg_error = fgPpplInit(ccrun.fg_limits,
                          ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                          &ccpars_pppl.config,
                          ccpars_global.run_delay,
                          ccpars_pppl.initial_ref,
                          &ccpars_pppl.pars,
                          fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        ccTestPrintError("failed to initialise PPPL (error_index %u) : %s : %g,%g,%g,%g",
                fg_meta->error.index,ccRefErrMsg(fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitTABLE(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Link table data to table config structure

    ccpars_table.config.ref             = ccpars_table.ref;
    ccpars_table.config.time            = ccpars_table.time;
    ccpars_table.config.ref_n_elements  = table_pars[0].num_elements;
    ccpars_table.config.time_n_elements = table_pars[1].num_elements;

    // Try to initialise the TABLE

    fg_error = fgTableInit(ccrun.fg_limits,
                           ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                           &ccpars_table.config,
                           ccpars_global.run_delay,
                           conv.iter_period,
                           &ccpars_table.pars,
                           fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        ccTestPrintError("failed to initialise TABLE (point %u) : %s : %g,%g,%g,%g",
                fg_meta->error.index,ccRefErrMsg(fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitSTEPS(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Check that the TEST configuration isn't already incompatible

    if(ccpars_test.config.type != FG_TEST_STEPS && ccpars_test.config.type != FG_TEST_UNDEFINED)
    {
        ccTestPrintError("failed to initialise STEPS because another test function was already armed");
        return(EXIT_FAILURE);
    }

    // Define type of test function

    ccpars_test.config.type = FG_TEST_STEPS;

    // Try to initialise the STEPS

    fg_error = fgTestInit(ccrun.fg_limits,
                          ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                          &ccpars_test.config,
                          ccpars_global.run_delay,
                          ccpars_test.initial_ref,
                          &ccpars_test.pars,
                          fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        ccTestPrintError("failed to initialise STEPS : %s : %g,%g,%g,%g",ccRefErrMsg(fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitSQUARE(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Check that the TEST configuration isn't already incompatible

    if(ccpars_test.config.type != FG_TEST_SQUARE && ccpars_test.config.type != FG_TEST_UNDEFINED)
    {
        ccTestPrintError("failed to initialise SQUARE because another test function was already armed");
        return(EXIT_FAILURE);
    }

    // Define type of test function

    ccpars_test.config.type = FG_TEST_SQUARE;

    // Try to initialise the SQUARE

    fg_error = fgTestInit(ccrun.fg_limits,
                          ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                          &ccpars_test.config,
                          ccpars_global.run_delay,
                          ccpars_test.initial_ref,
                          &ccpars_test.pars,
                          fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        ccTestPrintError("failed to initialise SQUARE : %s : %g,%g,%g,%g",ccRefErrMsg(fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitSINE(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Check that the TEST configuration isn't already incompatible

    if(ccpars_test.config.type != FG_TEST_SINE && ccpars_test.config.type != FG_TEST_UNDEFINED)
    {
        ccTestPrintError("failed to initialise SINE because another test function was already armed");
        return(EXIT_FAILURE);
    }

    // Define type of test function

    ccpars_test.config.type = FG_TEST_SINE;

    // Try to initialise the SINE

    fg_error = fgTestInit(ccrun.fg_limits,
                          ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                          &ccpars_test.config,
                          ccpars_global.run_delay,
                          ccpars_test.initial_ref,
                          &ccpars_test.pars,
                          fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        ccTestPrintError("failed to initialise SINE : %s : %g,%g,%g,%g",ccRefErrMsg(fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitCOSINE(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Check that the TEST configuration isn't already incompatible

    if(ccpars_test.config.type != FG_TEST_COSINE && ccpars_test.config.type != FG_TEST_UNDEFINED)
    {
        ccTestPrintError("failed to initialise COSINE because another test function was already armed");
        return(EXIT_FAILURE);
    }

    // Define type of test function

    ccpars_test.config.type = FG_TEST_COSINE;

    // Try to initialise the COSINE

    fg_error = fgTestInit(ccrun.fg_limits,
                          ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                          &ccpars_test.config,
                          ccpars_global.run_delay,
                          ccpars_test.initial_ref,
                          &ccpars_test.pars,
                          fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        ccTestPrintError("failed to initialise COSINE : %s : %g,%g,%g,%g",ccRefErrMsg(fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitLTRIM(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Check that the TRIM configuration isn't already incompatible

    if(ccpars_trim.config.type != FG_TRIM_LINEAR && ccpars_trim.config.type != FG_TRIM_UNDEFINED)
    {
        ccTestPrintError("failed to initialise LTRIM because CTRIM already armed");
        return(EXIT_FAILURE);
    }

    // Define type of trim function

    ccpars_trim.config.type = FG_TRIM_LINEAR;

    // Try to initialise the LTRIM

    fg_error = fgTrimInit(ccrun.fg_limits,
                          ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                          &ccpars_trim.config,
                          ccpars_global.run_delay,
                          ccpars_trim.initial_ref,
                          &ccpars_trim.pars,
                          fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        ccTestPrintError("failed to initialise LTRIM : %s : %g,%g,%g,%g",ccRefErrMsg(fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitCTRIM(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Check that the TRIM configuration isn't already incompatible

    if(ccpars_trim.config.type != FG_TRIM_CUBIC && ccpars_trim.config.type != FG_TRIM_UNDEFINED)
    {
        ccTestPrintError("failed to initialise CTRIM because LTRIM already armed");
        return(EXIT_FAILURE);
    }

    // Define type of trim function

    ccpars_trim.config.type = FG_TRIM_CUBIC;

    // Try to initialise the CTRIM

    fg_error = fgTrimInit(ccrun.fg_limits,
                          ccRefLimitsPolarity(ccpars_limits.invert, ccpars_load.pol_swi_auto),
                          &ccpars_trim.config,
                          ccpars_global.run_delay,
                          ccpars_trim.initial_ref,
                          &ccpars_trim.pars,
                          fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        ccTestPrintError("failed to initialise CTRIM : %s : %g,%g,%g,%g",ccRefErrMsg(fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
// EOF


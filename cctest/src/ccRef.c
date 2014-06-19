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
    case FG_OUT_OF_VOLTAGE_LIMITS:      return("out of voltage limits");
    }

    return("unknown error");
}
/*---------------------------------------------------------------------------------------------------------*/
static enum fg_limits_polarity ccRefLimitsPolarity(uint32_t invert_limits, uint32_t pol_swi_auto)
/*---------------------------------------------------------------------------------------------------------*/
{
    if(pol_swi_auto == CC_ENABLED)
    {
        return(FG_LIMITS_POL_AUTO);        // Limits should be tested based upon the polarity of the function
    }
    else if(invert_limits == CC_ENABLED)
    {
        return(FG_LIMITS_POL_NEGATIVE);    // Limits should be inverted
    }

    return(FG_LIMITS_POL_NORMAL);          // Normal limits with no manipulation
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitSTART(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Check that FG_LIMITS are ENABLED and REG_MODE is NOT VOLTAGE

    if(ccrun.fg_limits == NULL)
    {
        ccTestPrintError("START function requires GLOBAL FG_LIMITS to be ENABLED");
        return(EXIT_FAILURE);
    }

    if(ccrun.fg_limits->user_data == REG_VOLTAGE)
    {
        ccTestPrintError("START function requires GLOBAL REG_MODE to be CURRENT or FIELD");
        return(EXIT_FAILURE);
    }

    // Try to initialise a PLEP from zero to the final reference for the start-up.  This is to check the
    // limits.  The PLEP parameters will be reinitialised when the measurement crosses the closeloop level.

    ccpars_start.config.linear_rate = ccrun.fg_limits->rate;

    fg_error = fgPlepInit(ccrun.fg_limits,
                          ccRefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto),
                          &ccpars_start.config,
                          ccpars_global.run_delay, 0.0,
                          &ccpars_start.plep_pars, fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        ccTestPrintError("failed to initialise PLEP for START (segment %u) : %s : %g,%g,%g,%g",
                fg_meta->error.index,ccRefErrMsg(fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    // Check that closeloop level is within reasonable limits

    if(ccpars_start.closeloop_level < (0.2 * ccpars_start.config.final) ||
       ccpars_start.closeloop_level > (0.8 * ccpars_start.config.final))
    {
        ccTestPrintError("start CLOSELOOP_LEVEL (%.7E) must be 20-80%% of FINAL_REF (%.7E)",
                ccpars_start.closeloop_level,ccpars_start.config.final);
        return(EXIT_FAILURE);
    }

    // Start with feedforward enabled and V_REF = 0

    ccrun.feedforward_v_ref   = 0.0;
    ccrun.feedforward_control = 1;

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefInitPLEP(struct fg_meta *fg_meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Try to initialise the PLEP

    fg_error = fgPlepInit(ccrun.fg_limits,
                          ccRefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto),
                          &ccpars_plep.config,
                          ccpars_global.run_delay, ccpars_plep.initial_ref,
                          &ccpars_plep.plep_pars, fg_meta);

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
            fg_error = FG_BAD_PARAMETER;
        }
        else
        {
            fgResetMeta(fg_meta, NULL, ccpars_ramp.initial_ref);

            fgRampCalc(&ccpars_ramp.config,
                       &ccpars_ramp.ramp_pars,
                        ccpars_global.run_delay,
                        ccpars_ramp.initial_ref,
                        ccpars_ramp.initial_rate,
                        0.0,
                        fg_meta);
        }
    }
    else
    {
        // If initial rate is zero, use normal initialisation with limits checking

        fg_error = fgRampInit(ccrun.fg_limits,
                              ccRefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto),
                              &ccpars_ramp.config,
                              ccpars_global.run_delay,
                              ccpars_ramp.initial_ref,
                              &ccpars_ramp.ramp_pars,
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
                          ccRefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto),
                          &ccpars_pppl.config,
                          ccpars_global.run_delay, ccpars_pppl.initial_ref,
                          &ccpars_pppl.pppl_pars, fg_meta);

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
                           ccRefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto),
                           &ccpars_table.config,
                           ccpars_global.run_delay, reg.iter_period,
                           &ccpars_table.table_pars, fg_meta);

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
                          ccRefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto),
                          &ccpars_test.config,
                          ccpars_global.run_delay, ccpars_test.initial_ref,
                          &ccpars_test.test_pars, fg_meta);

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
                          ccRefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto),
                          &ccpars_test.config,
                          ccpars_global.run_delay, ccpars_test.initial_ref,
                          &ccpars_test.test_pars, fg_meta);

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
                          ccRefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto),
                          &ccpars_test.config,
                          ccpars_global.run_delay, ccpars_test.initial_ref,
                          &ccpars_test.test_pars, fg_meta);

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
                          ccRefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto),
                          &ccpars_test.config,
                          ccpars_global.run_delay, ccpars_test.initial_ref,
                          &ccpars_test.test_pars, fg_meta);

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
                          ccRefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto),
                          &ccpars_trim.config,
                          ccpars_global.run_delay, ccpars_trim.initial_ref,
                          &ccpars_trim.trim_pars, fg_meta);

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
                          ccRefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto),
                          &ccpars_trim.config,
                          ccpars_global.run_delay, ccpars_trim.initial_ref,
                          &ccpars_trim.trim_pars, fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        ccTestPrintError("failed to initialise CTRIM : %s : %g,%g,%g,%g",ccRefErrMsg(fg_error),
                fg_meta->error.data[0],fg_meta->error.data[1],fg_meta->error.data[2],fg_meta->error.data[3]);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccRefStartGen(struct fg_plep_pars *pars, const double *time, float *ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function implements an open loop start by applying a feedfoward voltage reference until the
  measurement (field or current) rises above a threshold.  It then closes the loop and initialises
  a PLEP function to take the reference smoothly to the start plateau.  This function is modelled on
  the libfg functions but cannot be part of libfg because it has to manipulate the feedforward control
  of the regulation algorithm.
\*---------------------------------------------------------------------------------------------------------*/
{
    struct fg_meta fg_meta;     // Required for call to fgPlepCalc() but not used

    // If within the open loop period with feedforward voltage reference

    if(ccrun.feedforward_control == 1)
    {
        if(*time < ccpars_global.run_delay)
        {
            // Wait with zero voltage reference until end of RUN_DELAY

            *ref = 0.0;

            return(0);          // 0 means function not finished
        }
        else if(reg.rst_vars.meas[0] < ccpars_start.closeloop_level)
        {
            // Apply feedforward voltage reference to start converter in openloop

            ccrun.feedforward_v_ref = ccpars_start.feedforward_v_ref;

            *ref = 0.0;

            return(0);          // 0 means function not finished
        }
        else
        {
            // Calculate rate of rise

            ccpars_start.config.linear_rate = regRstDeltaRefRT(&reg.rst_vars) / reg.period;

            // Initialise PLEP function to continue this ramp rate up to the start plateau

            fgPlepCalc(&ccpars_start.config,
                        pars,
                        reg.time,
                        regRstPrevRefRT(&reg.rst_vars),
                        ccpars_start.config.linear_rate,
                        &fg_meta);

            // Close the loop with the reference continuing with the same ramp rate

            ccrun.feedforward_control = 0;
        }
    }

    // Closed loop running - use PLEP function to ramp to start reference

    return(fgPlepGen(pars,time,ref));
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccRefCheckConverterLimits(struct fg_limits *limits, uint32_t invert_limits,
                                        float ref, float rate, float acceleration, struct fg_meta *meta)
/*---------------------------------------------------------------------------------------------------------*/
{
    float v_ref;
    float i_ref;

    // If reference is in volts, then no additional converter limits check is needed (or possible)

    if(limits->user_data == REG_VOLTAGE)
    {
        return(FG_OK);
    }

    // If function is in gauss, then calculate corresponding current and rate of change of current

    if(limits->user_data == REG_FIELD)
    {
        i_ref  = regLoadFieldToCurrentRT(&reg.load_pars, ref);
        rate  *= i_ref / ref;
    }
    else
    {
        i_ref = ref;
    }

    // Use load model to estimate voltage required for this current and rate of change of current

    v_ref = i_ref * reg.load_pars.ohms +
            rate  * reg.load_pars.henrys * regLoadSatFactorRT(&reg.load_pars, i_ref);

    // Calculate the voltage limits for the current i_ref, taking into account invert_limits

    regLimRefSetInvertLimits(&ccrun.fg_lim_v_ref, invert_limits);

    regLimVrefCalcRT(&ccrun.fg_lim_v_ref, i_ref);

    // Check v_ref against voltage limits, inverted if required

    if(invert_limits == 0)
    {
        // Check estimated voltage required against estimated voltage available

        if(v_ref < ccrun.fg_lim_v_ref.min_clip || v_ref > ccrun.fg_lim_v_ref.max_clip)
        {
            meta->error.data[0] = ccrun.fg_lim_v_ref.min_clip;
            meta->error.data[1] = v_ref;
            meta->error.data[2] = ccrun.fg_lim_v_ref.max_clip;

            return(FG_OUT_OF_VOLTAGE_LIMITS);
        }
    }
    else
    {
        // Check estimated voltage required against estimated voltage available using inverted limits

        if(v_ref < -ccrun.fg_lim_v_ref.max_clip || v_ref > -ccrun.fg_lim_v_ref.min_clip)
        {
            meta->error.data[0] = -ccrun.fg_lim_v_ref.max_clip;
            meta->error.data[1] = v_ref;
            meta->error.data[2] = -ccrun.fg_lim_v_ref.min_clip;
            meta->error.data[3] = 1;

            return(FG_OUT_OF_VOLTAGE_LIMITS);
        }
    }

    return(FG_OK);
}
// EOF


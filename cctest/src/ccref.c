/*---------------------------------------------------------------------------------------------------------*\
  File:     ccref.c                                                                     Copyright CERN 2011

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

#include "ccpars.h"
#include "ccref.h"

/*---------------------------------------------------------------------------------------------------------*/
static char * ccrefErrMsg(enum fg_error fg_error)
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
void ccrefFuncType(char *arg)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct ccpars_enum  *par_enum;

    if(ccpars_global.function != FG_NONE)
    {
        fputs("Error : function type (-f) already defined global parameters file\n",stderr);
        exit(EXIT_FAILURE);
    }

    for(par_enum = function_type ; par_enum->string != NULL && strcasecmp(par_enum->string,arg) != 0 ; par_enum++);

    if(par_enum->string == NULL)
    {
        fprintf(stderr,"Error: %s is not a known function type\n",arg);
        exit(EXIT_FAILURE);
    }

    ccpars_global.function = par_enum->value;
}
/*---------------------------------------------------------------------------------------------------------*/
static enum fg_limits_polarity ccrefLimitsPolarity(uint32_t invert_limits, uint32_t pol_swi_auto)
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
void ccrefInitSTART(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Try to initialise a PLEP from zero to the final reference for the start-up.  This is to check the
    // limits.  The PLEP parameters will be reinitialised when the measurement crosses the closeloop level.

    ccpars_start.config.linear_rate = ccpars_limits.fg->rate;


    fg_error = fgPlepInit(ccpars_limits.fg,
                          ccrefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto), 
                          &ccpars_start.config,
                          ccpars_global.run_delay, 0.0,
                          &ccpars_start.plep_pars, &fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        fprintf(stderr,"Error : Failed to initialise PLEP for START (segment %u) : %s\n",
                fg_meta.error.index,ccrefErrMsg(fg_error));
        exit(EXIT_FAILURE);
    }

    // Check that closeloop level is within reasonable limits

    if(ccpars_start.closeloop_level < (0.2 * ccpars_start.config.final) ||
       ccpars_start.closeloop_level > (0.8 * ccpars_start.config.final))
    {
        fprintf(stderr,"Error : Start CLOSELOOP_LEVEL (%.7E) must be 20-80%% of FINAL_REF (%.7E)\n",
                ccpars_start.closeloop_level,ccpars_start.config.final);
        exit(EXIT_FAILURE);
    }

    // Start with feedforward enabled and V_REF = 0

    ccpars_reg.feedforward_v_ref   = 0.0;
    ccpars_reg.feedforward_control = 1;
}
/*---------------------------------------------------------------------------------------------------------*/
void ccrefInitPLEP(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Try to initialise the PLEP

    fg_error = fgPlepInit(ccpars_limits.fg, 
                          ccrefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto), 
                          &ccpars_plep.config,
                          ccpars_global.run_delay, ccpars_plep.initial_ref,
                          &ccpars_plep.plep_pars, &fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        fprintf(stderr,"Error : Failed to initialise PLEP (segment %u) : %s\n",
                fg_meta.error.index,ccrefErrMsg(fg_error));
        exit(EXIT_FAILURE);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccrefInitRAMP(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Try to initialise the RAMP

    fg_error = fgRampInit(ccpars_limits.fg, 
                          ccrefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto), 
                          &ccpars_ramp.config,
                          ccpars_global.run_delay, ccpars_ramp.initial_ref,
                          &ccpars_ramp.ramp_pars, &fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        fprintf(stderr,"Error : Failed to initialise RAMP (segment %u) : %s\n",
                fg_meta.error.index,ccrefErrMsg(fg_error));
        exit(EXIT_FAILURE);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccrefInitPPPL(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Set up pppl array lengths

    ccpars_pppl.config.numels_acceleration1 = pppl_pars_list[1].num_values;
    ccpars_pppl.config.numels_acceleration2 = pppl_pars_list[2].num_values;
    ccpars_pppl.config.numels_acceleration3 = pppl_pars_list[3].num_values;
    ccpars_pppl.config.numels_rate2         = pppl_pars_list[4].num_values;
    ccpars_pppl.config.numels_rate4         = pppl_pars_list[5].num_values;
    ccpars_pppl.config.numels_ref4          = pppl_pars_list[6].num_values;
    ccpars_pppl.config.numels_duration4     = pppl_pars_list[7].num_values;

    // Try to initialise the PPPL

    fg_error = fgPpplInit(ccpars_limits.fg, 
                          ccrefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto), 
                          &ccpars_pppl.config,
                          ccpars_global.run_delay, ccpars_pppl.initial_ref,
                          &ccpars_pppl.pppl_pars, &fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        fprintf(stderr,"Error : Failed to initialise PPPL (segment %u) : %s\n",
                fg_meta.error.index,ccrefErrMsg(fg_error));
        exit(EXIT_FAILURE);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccrefInitTABLE(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Link table data to table config structure

    ccpars_table.config.ref             = ccpars_table.ref;
    ccpars_table.config.time            = ccpars_table.time;
    ccpars_table.config.ref_n_elements  = table_pars_list[0].num_values;
    ccpars_table.config.time_n_elements = table_pars_list[1].num_values;

    // Try to initialise the TABLE

    fg_error = fgTableInit(ccpars_limits.fg, 
                           ccrefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto), 
                           &ccpars_table.config,
                           ccpars_global.run_delay, reg.iter_period,
                           &ccpars_table.table_pars, &fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        fprintf(stderr,"Error : Failed to initialise TABLE (point %u) : %s\n",
                fg_meta.error.index,ccrefErrMsg(fg_error));
        exit(EXIT_FAILURE);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccrefInitSTEPS(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Define type of test function

    ccpars_test.config.type = FG_TEST_STEPS;

    // Try to initialise the STEPS

    fg_error = fgTestInit(ccpars_limits.fg, 
                          ccrefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto), 
                          &ccpars_test.config,
                          ccpars_global.run_delay, ccpars_test.initial_ref,
                          &ccpars_test.test_pars, &fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        fprintf(stderr,"Error : Failed to initialise STEPS : %s\n",ccrefErrMsg(fg_error));
        exit(EXIT_FAILURE);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccrefInitSQUARE(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Define type of test function

    ccpars_test.config.type = FG_TEST_SQUARE;

    // Try to initialise the SQUARE

    fg_error = fgTestInit(ccpars_limits.fg, 
                          ccrefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto), 
                          &ccpars_test.config,
                          ccpars_global.run_delay, ccpars_test.initial_ref,
                          &ccpars_test.test_pars, &fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        fprintf(stderr,"Error : Failed to initialise SQUARE : %s\n",ccrefErrMsg(fg_error));
        exit(EXIT_FAILURE);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccrefInitSINE(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Define type of test function

    ccpars_test.config.type = FG_TEST_SINE;

    // Try to initialise the SINE

    fg_error = fgTestInit(ccpars_limits.fg, 
                          ccrefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto), 
                          &ccpars_test.config,
                          ccpars_global.run_delay, ccpars_test.initial_ref,
                          &ccpars_test.test_pars, &fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        fprintf(stderr,"Error : Failed to initialise SINE : %s\n",ccrefErrMsg(fg_error));
        exit(EXIT_FAILURE);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccrefInitCOSINE(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Define type of test function

    ccpars_test.config.type = FG_TEST_COSINE;

    // Try to initialise the COSINE

    fg_error = fgTestInit(ccpars_limits.fg, 
                          ccrefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto), 
                          &ccpars_test.config,
                          ccpars_global.run_delay, ccpars_test.initial_ref,
                          &ccpars_test.test_pars, &fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        fprintf(stderr,"Error : Failed to initialise COSINE : %s\n",ccrefErrMsg(fg_error));
        exit(EXIT_FAILURE);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccrefInitLTRIM(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Define type of trim function

    ccpars_trim.config.type = FG_TRIM_LINEAR;

    // Try to initialise the LTRIM

    fg_error = fgTrimInit(ccpars_limits.fg, 
                          ccrefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto), 
                          &ccpars_trim.config,
                          ccpars_global.run_delay, ccpars_trim.initial_ref,
                          &ccpars_trim.trim_pars, &fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        fprintf(stderr,"Error : Failed to initialise LTRIM : %s\n",ccrefErrMsg(fg_error));
        exit(EXIT_FAILURE);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccrefInitCTRIM(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    enum fg_error fg_error;

    // Define type of trim function

    ccpars_trim.config.type = FG_TRIM_CUBIC;

    // Try to initialise the CTRIM

    fg_error = fgTrimInit(ccpars_limits.fg, 
                          ccrefLimitsPolarity(ccpars_limits.invert_limits, ccpars_load.pol_swi_auto), 
                          &ccpars_trim.config,
                          ccpars_global.run_delay, ccpars_trim.initial_ref,
                          &ccpars_trim.trim_pars, &fg_meta);

    // Report error if initialisation fails

    if(fg_error != FG_OK)
    {
        fprintf(stderr,"Error : Failed to initialise CTRIM : %s\n",ccrefErrMsg(fg_error));
        exit(EXIT_FAILURE);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccrefStartGen(struct fg_plep_pars *pars, const double *time, float *ref)
/*---------------------------------------------------------------------------------------------------------*\
  This function implements an open loop start by applying a feedfoward voltage reference until the
  measurement (field or current) rises above a threshold.  It then closes the loop and initialises
  a PLEP function to take the reference smoothly to the start plateau.  This function is modelled on
  the libfg functions but cannot be part of libfg because it has to manipulate the feedforward control
  of the regulation algorithm.
\*---------------------------------------------------------------------------------------------------------*/
{
    // If within the open loop period with feedforward voltage reference

    if(ccpars_reg.feedforward_control == 1)
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

            ccpars_reg.feedforward_v_ref = ccpars_start.feedforward_v_ref;

            *ref = 0.0;

            return(0);          // 0 means function not finished
        }
        else
        {
            // Calculate rate of rise

            ccpars_start.config.linear_rate = (reg.rst_vars.ref[1] - reg.rst_vars.ref[2]) / reg.cl_period;

            // Initialise PLEP function to continue this ramp rate up to the start plateau

            fgPlepCalc(&ccpars_start.config,
                        pars,
                        ccpars_reg.time,
                        reg.rst_vars.ref[1],
                        ccpars_start.config.linear_rate,
                        &fg_meta);

            // Close the loop with the reference continuing with the same ramp rate

            ccpars_reg.feedforward_control = 0;
        }
    }

    // Closed loop running - use PLEP function to ramp to start reference

    return(fgPlepGen(pars,time,ref));
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccrefCheckConverterLimits(struct fg_limits *limits, uint32_t invert_limits, 
                                        float ref, float rate, float acceleration)
/*---------------------------------------------------------------------------------------------------------*/
{
    float v_ref;
    float i_ref;

    // If function is in gauss, then calculate corresponding current and rate of change of current

    if(ccpars_global.units == REG_FIELD)
    {
        i_ref  = regLoadFieldToCurrent(&reg_pars.load_pars, ref);
        rate  *= i_ref / ref;
    }
    else
    {
        i_ref = ref;
    }

    // Use load model to estimate voltage required for this current and rate of change of current

    v_ref = i_ref * reg_pars.load_pars.ohms +
            rate  * reg_pars.load_pars.henrys * regLoadCalcSatFactor(&reg_pars.load_pars, i_ref);

    // Calculate the voltage limits for the current i_ref, taking into account invert_limits

    regLimRefSetInvertLimits(&ccpars_limits.fg_v_ref, invert_limits);

    regLimVrefCalc(&ccpars_limits.fg_v_ref, i_ref);

    // Check v_ref against voltage limits, inverted if required

    if(invert_limits == 0)
    {
        // Check estimated voltage required against estimated voltage available

        if(v_ref < ccpars_limits.fg_v_ref.min_clip || v_ref > ccpars_limits.fg_v_ref.max_clip)
        {
            return(FG_OUT_OF_VOLTAGE_LIMITS);
        }
    }
    else
    {
        // Check estimated voltage required against estimated voltage available using inverted limits

        if(v_ref < -ccpars_limits.fg_v_ref.max_clip || v_ref > -ccpars_limits.fg_v_ref.min_clip)
        {
            return(FG_OUT_OF_VOLTAGE_LIMITS);
        }
    }

    return(FG_OK);
}
// EOF


/*---------------------------------------------------------------------------------------------------------*\
  File:     ccPars.c                                                                    Copyright CERN 2014

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

  Purpose:  Converter controls libraries test program parameter parsing functions

  Author:   Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <float.h>
#include <errno.h>

// Include cctest program header files

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRef.h"
#include "ccRun.h"

// Constants

#define INT_FORMAT          "% d"
#define FLOAT_FORMAT        "% .6E"
#define DOUBLE_FORMAT       "% 20.16E"

/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccParsGet(char *cmd_name, struct ccpars *par, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to interpret arguments remaining on the input line as values belonging to
  a command parameter.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t             num_elements;
    uint32_t             int_value;
    double               double_value;
    char                *remaining_arg;
    char                *arg;
    size_t               arg_len;
    struct ccpars_enum  *par_enum;
    struct ccpars_enum  *par_enum_matched;


    // Reset errno because strtod does not set it to zero on success

    errno = 0;

    // Try to parse the arguments to set the parameter values

    num_elements = 0;

    while((arg = ccTestGetArgument(remaining_line)) != NULL)
    {
        if(num_elements >= par->max_num_elements)
        {
            ccTestPrintError("too many values for %s %s (%u max)",
                    cmd_name,par->name,par->max_num_elements);
            return(EXIT_FAILURE);
        }

        arg_len = strlen(arg);

        switch(par->type)
        {
        case PAR_UNSIGNED:

            int_value = strtoul(arg, &remaining_arg, 10);

            if(*remaining_arg != '\0' || errno != 0)
            {
                ccTestPrintError("invalid integer for %s %s[%u]: '%s'",
                        cmd_name, par->name, num_elements, arg);
                return(EXIT_FAILURE);
            }

            par->value_p.i[num_elements] = int_value;
            break;

        case PAR_FLOAT:

            double_value = strtod(arg, &remaining_arg);

            // Protect against double value that cannot fit in a float

            if(double_value >= FLT_MAX || double_value <= -FLT_MAX)
            {
                errno = ERANGE;
            }

            if(*remaining_arg != '\0' || errno != 0)
            {
                ccTestPrintError("invalid float for %s %s[%u]: '%s'",
                        cmd_name, par->name, num_elements, arg);
                return(EXIT_FAILURE);
            }

            par->value_p.f[num_elements] = (float)double_value;
            break;

        case PAR_DOUBLE:

            double_value = strtod(arg, &remaining_arg);

            if(*remaining_arg != '\0' || errno != 0)
            {
                ccTestPrintError("invalid double for %s %s[%u]: '%s'",
                        cmd_name, par->name, num_elements, arg);
                return(EXIT_FAILURE);
            }

            par->value_p.d[num_elements] = double_value;
            break;

        case PAR_STRING:   // STRING parameters must be scalar (max_num_element = 1)

            // Fatal error if string parameter is an array

            if(par->max_num_elements > 1)
            {
                printf("Fatal - %s %s is a STRING parameter: max_num_elements is %u but must be 1\n",
                       cmd_name, par->name, par->max_num_elements);
                exit(EXIT_FAILURE);
            }

            // Limit string lengths because they are used in path names

            if(arg_len > 32)
            {
                ccTestPrintError("invalid string length (%u) for '%s...' (32 max)",ccTestAbbreviatedArg(arg));
            }

            // Free and reallocate space for string argument
            // On first call, the point is NULL which is ignored by free()

            free(*par->value_p.s);
            *par->value_p.s = strcpy(malloc(arg_len+1),arg);
            break;

        case PAR_ENUM:

            // Compare first argument against list of enum strings for the parameter

            for(par_enum = par->ccpars_enum, par_enum_matched = NULL ; par_enum->string != NULL ; par_enum++)
            {
                // If argument matches start or all of an enum string

                if(strncasecmp(par_enum->string, arg, arg_len) == 0)
                {
                    // If first match, remember enum

                    if(par_enum_matched == NULL)
                    {
                        par_enum_matched = par_enum;

                        // If match is exact then break out of search

                        if(strcasecmp(par_enum->string, arg) == 0)
                        {
                            break;
                        }
                    }
                    else // else second match so report error
                    {
                        ccTestPrintError("ambiguous enum for %s %s[%u]: '%s'",
                                         cmd_name, par->name, num_elements, arg);
                        return(EXIT_FAILURE);
                    }
                }
            }

            if(par_enum_matched == NULL)
            {
                ccTestPrintError("unknown enum for %s %s[%u]: '%s'",
                                 cmd_name, par->name, num_elements, ccTestAbbreviatedArg(arg));
                return(EXIT_FAILURE);
            }

            par->value_p.i[num_elements] = par_enum_matched->value;
            break;
        }

        par->num_elements = ++num_elements;
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
char * ccParsEnumString(struct ccpars_enum *par_enum, uint32_t value)
/*---------------------------------------------------------------------------------------------------------*\
  This function will return the string corresponding to the enum
\*---------------------------------------------------------------------------------------------------------*/
{
    while(par_enum->string != NULL && par_enum->value != value)
    {
        par_enum++;
    }

    return(par_enum->string != NULL ? par_enum->string : "invalid");
}
/*---------------------------------------------------------------------------------------------------------*/
void ccParsPrintAll(FILE *f, char *cmd_name, struct ccpars *par)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print the name and value(s) for all parameters for one command
\*---------------------------------------------------------------------------------------------------------*/
{
    while(par->name != NULL)
    {
        ccParsPrint(f, cmd_name, par++);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccParsPrint(FILE *f, char *cmd_name, struct ccpars *par)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print the name and value(s) for one parameter
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t            idx;

    fprintf(f,"%-*s %-*s",CC_MAX_CMD_NAME_LEN,cmd_name,CC_MAX_PAR_NAME_LEN,par->name);

    if(par->num_elements > 0)
    {
        for( idx = 0 ; idx < par->num_elements ; idx++)
        {
            if(idx > 0)
            {
                fputc(' ',f);
            }

            switch(par->type)
            {
            case PAR_UNSIGNED:

                fprintf(f, INT_FORMAT, par->value_p.i[idx]);
                break;

            case PAR_FLOAT:

                fprintf(f, FLOAT_FORMAT, par->value_p.f[idx]);
                break;

            case PAR_DOUBLE:

                fprintf(f, DOUBLE_FORMAT, par->value_p.d[idx]);
                break;

            case PAR_STRING:

                fprintf(f," %s",par->value_p.s[idx]);
                break;

            case PAR_ENUM:

                fprintf(f," %s",ccParsEnumString(par->ccpars_enum, par->value_p.i[idx]));
                break;
            }
        }
    }
    fputc('\n',f);
}
/*---------------------------------------------------------------------------------------------------------*/
static char * ccParsDebugLabel(char *prefix, char *variable)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    idx;
    static char debug_label[PARS_INDENT+1];
    char       *label = debug_label;

    // Prefix

    for(idx = 0 ; idx < PARS_INDENT && *prefix ; idx++)
    {
        *(label++) = *(prefix++);
    }

    // Separator

    if(idx < PARS_INDENT)
    {
        *(label++) = ' ';
        idx++;
    }

    // Variable name

    for(; idx < PARS_INDENT && *variable ; idx++)
    {
        *(label++) = *(variable++);
    }

    // Trailing spaces

    while(idx++ < PARS_INDENT)
    {
        *(label++) = ' ';
    }

    *(label++) = '\0';

    return(debug_label);
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccParsPrintDebugLoad(FILE *f, char *prefix, struct reg_load_pars *load_pars)
/*---------------------------------------------------------------------------------------------------------*/
{
    // Report internally calculated load parameters

    fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "ohms_ser"),   load_pars->ohms_ser);
    fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "ohms_par"),   load_pars->ohms_par);
    fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "ohms_mag"),   load_pars->ohms_mag);
    fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "henrys"),     load_pars->henrys);
    fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "inv_henrys"), load_pars->inv_henrys);
    fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "ohms"),       load_pars->ohms);
    fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "ohms1"),      load_pars->ohms1);
    fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "ohms2"),      load_pars->ohms2);
    fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "tc"),         load_pars->tc);
    fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "gain0"),      load_pars->gain0);
    fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "gain1"),      load_pars->gain1);
    fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "gain2"),      load_pars->gain2);
    fprintf(f,"%s" FLOAT_FORMAT "\n\n",ccParsDebugLabel(prefix, "gain3"),      load_pars->gain3);

    if(load_pars->sat.i_end > 0.0)
    {
        fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "sat.henrys"),   load_pars->sat.henrys);
        fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "sat.i_delta"),  load_pars->sat.i_delta);
        fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "sat.b_end"),    load_pars->sat.b_end);
        fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "sat.b_factor"), load_pars->sat.b_factor);
        fprintf(f,"%s" FLOAT_FORMAT "\n",  ccParsDebugLabel(prefix, "sat.l_rate"),   load_pars->sat.l_rate);
        fprintf(f,"%s" FLOAT_FORMAT "\n\n",ccParsDebugLabel(prefix, "sat.l_clip"),   load_pars->sat.l_clip);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccParsPrintDebugReg(FILE *f, char *prefix, struct reg_conv_signal *reg_signal)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t i;

    fprintf(f,"%s" INT_FORMAT    "\n", ccParsDebugLabel(prefix, "alg_index"),           reg_signal->rst_pars->alg_index);
    fprintf(f,"%s" INT_FORMAT    "\n", ccParsDebugLabel(prefix, "dead_beat"),           reg_signal->rst_pars->dead_beat);
    fprintf(f,"%s" INT_FORMAT    "\n", ccParsDebugLabel(prefix, "jurys_result"),        reg_signal->rst_pars->jurys_result);
    fprintf(f,"%s" FLOAT_FORMAT  "\n", ccParsDebugLabel(prefix, "modulus_margin"),      reg_signal->rst_pars->modulus_margin);
    fprintf(f,"%s" FLOAT_FORMAT  "\n", ccParsDebugLabel(prefix, "modulus_margin_freq"), reg_signal->rst_pars->modulus_margin_freq);

    fprintf(f,"%s" DOUBLE_FORMAT "\n", ccParsDebugLabel(prefix, "pure_delay_periods"),  reg_signal->rst_pars->pure_delay_periods);
    fprintf(f,"%s" DOUBLE_FORMAT "\n", ccParsDebugLabel(prefix, "track_delay_periods"), reg_signal->rst_pars->track_delay_periods);
    fprintf(f,"%s" DOUBLE_FORMAT "\n", ccParsDebugLabel(prefix, "ref_delay_periods"),   reg_signal->rst_pars->ref_delay_periods);
    fprintf(f,"%s" DOUBLE_FORMAT "\n", ccParsDebugLabel(prefix, "t0_correction"),       reg_signal->rst_pars->t0_correction);

    fprintf(f,"%s" DOUBLE_FORMAT "\n", ccParsDebugLabel(prefix, "openloop_fwd_ref[0]"), reg_signal->rst_pars->openloop_forward.ref[0]);
    fprintf(f,"%s" DOUBLE_FORMAT "\n", ccParsDebugLabel(prefix, "openloop_fwd_ref[1]"), reg_signal->rst_pars->openloop_forward.ref[1]);
    fprintf(f,"%s" DOUBLE_FORMAT "\n", ccParsDebugLabel(prefix, "openloop_fwd_act[1]"), reg_signal->rst_pars->openloop_forward.act[1]);

    fprintf(f,"%s" DOUBLE_FORMAT "\n", ccParsDebugLabel(prefix, "openloop_rev_ref[1]"), reg_signal->rst_pars->openloop_reverse.ref[1]);
    fprintf(f,"%s" DOUBLE_FORMAT "\n", ccParsDebugLabel(prefix, "openloop_rev_act[0]"), reg_signal->rst_pars->openloop_reverse.act[0]);
    fprintf(f,"%s" DOUBLE_FORMAT "\n", ccParsDebugLabel(prefix, "openloop_rev_act[1]"), reg_signal->rst_pars->openloop_reverse.act[1]);

    for(i = 0 ; i < REG_N_RST_COEFFS ; i++)
    {
        fprintf(f,"%s" DOUBLE_FORMAT " " DOUBLE_FORMAT " " DOUBLE_FORMAT " "
                       DOUBLE_FORMAT " " DOUBLE_FORMAT " " DOUBLE_FORMAT "\n", ccParsDebugLabel(prefix, "R:S:T:A:B:AS+BR"),
                       reg_signal->rst_pars->rst.r[i],
                       reg_signal->rst_pars->rst.s[i],
                       reg_signal->rst_pars->rst.t[i],
                       reg_signal->rst_pars->a[i],
                       reg_signal->rst_pars->b[i],
                       reg_signal->rst_pars->asbr[i]);
    }

    fputc('\n',f);
}
/*---------------------------------------------------------------------------------------------------------*/
void ccParsPrintDebug(FILE *f)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t     i;

    if(ccpars_global.sim_load == REG_ENABLED)
    {
        ccParsPrintDebugLoad(f, "LOAD", &conv.load_pars);

        fprintf(f,"%s" INT_FORMAT "\n",    ccParsDebugLabel("SIMLOAD", "load_undersampled_flag"),
                conv.sim_load_pars.load_undersampled_flag);
        fprintf(f,"%s" FLOAT_FORMAT "\n\n",ccParsDebugLabel("SIMLOAD", "period_tc_ratio"),
                conv.sim_load_pars.period_tc_ratio);

        if(conv.sim_load_pars.tc_error != 0.0)
        {
            fprintf(f,"%s" FLOAT_FORMAT "\n",ccParsDebugLabel("SIMLOAD", "tc_error"), conv.sim_load_pars.tc_error);

            ccParsPrintDebugLoad(f, "SIMLOAD", &conv.sim_load_pars.load_pars);
        }

        // Report internally calculated voltage source parameters

        for(i = 0 ; i < REG_N_VS_SIM_COEFFS ; i++)
        {
            fprintf(f,"%-*s" DOUBLE_FORMAT "  " DOUBLE_FORMAT "\n", PARS_INDENT, "SIMVS num:den",
                     conv.sim_vs_pars.num[i],
                     conv.sim_vs_pars.den[i]);
        }

        fprintf(f,"%s"     INT_FORMAT   "\n",   ccParsDebugLabel("SIMVS", "vs_undersampled_flag"), conv.sim_vs_pars.vs_undersampled_flag);
        fprintf(f,"\n%-*s" FLOAT_FORMAT "\n",   PARS_INDENT, "SIMVS vs_delay_iters",       conv.sim_vs_pars.vs_delay_iters);
        fprintf(f,"%-*s"   FLOAT_FORMAT "\n\n", PARS_INDENT, "SIMVS gain",                 conv.sim_vs_pars.gain);


        // Report internally calculated field regulation parameters

        if(conv.b.reg_period == REG_ENABLED)
        {
            ccParsPrintDebugReg(f, "BREG", &conv.b);
        }

        // Report internally calculated current regulation parameters

        if(conv.i.regulation == REG_ENABLED)
        {
            ccParsPrintDebugReg(f, "IREG", &conv.i);
        }
    }

    // Report function meta data from last run

    for(i = 0 ; i < ccrun.num_functions ; i++)
    {
        fprintf(f,"\n%-*s %s\n",  PARS_INDENT, "REF function",
                            ccParsEnumString(function_type, ccpars_global.function[i]));
        fprintf(f,"%-*s %s\n",   PARS_INDENT, "REF reg_mode",
                            ccParsEnumString(reg_mode,      ccpars_global.reg_mode[i]));
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "REG ref_advance",     ccrun.func[i].ref_advance);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "REG max_abs_err",     ccrun.func[i].max_abs_err);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "FG_META duration",    ccrun.func[i].fg_meta.duration);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "FG_META range.start", ccrun.func[i].fg_meta.range.start);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "FG_META range.end",   ccrun.func[i].fg_meta.range.end);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "FG_META range.min",   ccrun.func[i].fg_meta.range.min);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "FG_META range.max",   ccrun.func[i].fg_meta.range.max);
    }
}
// EOF

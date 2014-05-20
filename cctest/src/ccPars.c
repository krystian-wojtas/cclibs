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
    double               double_value;
    char                *remaining_arg;
    char                *arg;
    size_t               arg_len;
    struct ccpars_enum  *par_enum;
    struct ccpars_enum  *par_enum_matched;

    // Reset errno because strtod does not set it to zero on success

    errno = 0;

    // Try to parse the arguments to set the parameter values

    par->num_elements = 0;

    while((arg = ccTestGetArgument(remaining_line)) != NULL)
    {
        if(par->num_elements >= par->max_num_elements)
        {
            ccTestPrintError("too many values for %s %s (%u max)",
                    cmd_name,par->name,par->max_num_elements);
            return(EXIT_FAILURE);
        }

        arg_len = strlen(arg);

        switch(par->type)
        {
        case PAR_UNSIGNED:

            par->value_p.i[par->num_elements] = strtoul(arg, &remaining_arg, 10);

            if(*remaining_arg != '\0' || errno != 0)
            {
                ccTestPrintError("invalid integer for %s %s[%u]: '%s' : %s (%d)",
                        cmd_name, par->name, par->num_elements, arg, strerror(errno), errno);
                return(EXIT_FAILURE);
            }
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
                ccTestPrintError("invalid float for %s %s[%u]: '%s' : %s (%d)",
                        cmd_name, par->name, par->num_elements, arg, strerror(errno), errno);
                return(EXIT_FAILURE);
            }

            par->value_p.f[par->num_elements] = (float)double_value;
            break;

        case PAR_DOUBLE:

            par->value_p.d[par->num_elements] = strtod(arg, &remaining_arg);

            if(*remaining_arg != '\0' || errno != 0)
            {
                ccTestPrintError("invalid double for %s %s[%u]: '%s' : %s (%d)",
                        cmd_name, par->name, par->num_elements, arg, strerror(errno), errno);
                return(EXIT_FAILURE);
            }
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
                    }
                    else // else second match so report error
                    {
                        ccTestPrintError("ambiguous enum for %s %s[%u]: '%s'",
                                         cmd_name, par->name, par->num_elements, arg);
                        return(EXIT_FAILURE);
                    }
                }
            }

            if(par_enum_matched == NULL)
            {
                ccTestPrintError("unknown enum for %s %s[%u]: '%s'",
                                 cmd_name, par->name, par->num_elements, ccTestAbbreviatedArg(arg));
                return(EXIT_FAILURE);
            }

            par->value_p.i[par->num_elements] = par_enum_matched->value;
            break;
        }

        par->num_elements++;
    }

    if(par->num_elements < par->min_num_elements)
    {
        ccTestPrintError("too few values for %s %s[%u]: %u of %u",
                         cmd_name, par->name, par->num_elements, par->num_elements, par->min_num_elements);
        return(EXIT_FAILURE);
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

    if(par->num_elements > 0)
    {
        fprintf(f,"%-*s %-*s",CC_MAX_CMD_NAME_LEN,cmd_name,CC_MAX_PAR_NAME_LEN,par->name);

        for( idx = 0 ; idx < par->num_elements ; idx++)
        {
            if(idx > 0)
            {
                fputc(',',f);
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

        fputc('\n',f);
    }
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
        *(label++) = ':';
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
    fprintf(f,"%s" FLOAT_FORMAT "\n\n",ccParsDebugLabel(prefix, "gain10"),     load_pars->gain10);

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
void ccParsPrintDebug(FILE *f)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t     i;

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        ccParsPrintDebugLoad(f, "LOAD", &reg_pars.load_pars);

        fprintf(f,"%s" INT_FORMAT "\n",    ccParsDebugLabel("SIMLOAD", "vs_undersampled_flag"),
                reg_pars.sim_load_pars.vs_undersampled_flag);
        fprintf(f,"%s" INT_FORMAT "\n",    ccParsDebugLabel("SIMLOAD", "load_undersampled_flag"),
                reg_pars.sim_load_pars.load_undersampled_flag);
        fprintf(f,"%s" FLOAT_FORMAT "\n\n",ccParsDebugLabel("SIMLOAD", "period_tc_ratio"),
                reg_pars.sim_load_pars.period_tc_ratio);

        if(reg_pars.sim_load_pars.tc_error != 0.0)
        {
            fprintf(f,"%s" FLOAT_FORMAT "\n",ccParsDebugLabel("SIMLOAD", "tc_error"), reg_pars.sim_load_pars.tc_error);

            ccParsPrintDebugLoad(f, "SIMLOAD", &reg_pars.sim_load_pars.load_pars);
        }

        // Report internally calculated voltage source parameters

        fprintf(f,"%-*s" FLOAT_FORMAT "," FLOAT_FORMAT "," FLOAT_FORMAT "," FLOAT_FORMAT "\n", PARS_INDENT, "SIMVS:numerator",
                     reg_pars.sim_vs_pars.num[0],
                     reg_pars.sim_vs_pars.num[1],
                     reg_pars.sim_vs_pars.num[2],
                     reg_pars.sim_vs_pars.num[3]);

        fprintf(f,"%-*s" FLOAT_FORMAT "," FLOAT_FORMAT "," FLOAT_FORMAT "," FLOAT_FORMAT "\n", PARS_INDENT, "SIMVS:denominator",
                     reg_pars.sim_vs_pars.den[0],
                     reg_pars.sim_vs_pars.den[1],
                     reg_pars.sim_vs_pars.den[2],
                     reg_pars.sim_vs_pars.den[3]);

        fprintf(f,"%-*s" FLOAT_FORMAT "\n",   PARS_INDENT, "SIMVS:step_rsp_time_iters",reg_pars.sim_vs_pars.step_rsp_time_iters);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n\n", PARS_INDENT, "SIMVS:gain",               reg_pars.sim_vs_pars.gain);

        // Report internally calculated field regulation parameters

        if(ccrun.breg_flag == 1)
        {
            fprintf(f,"%-*s" DOUBLE_FORMAT "\n", PARS_INDENT, "BREG:pure_delay_periods",
                               reg_pars.b_rst_pars.pure_delay_periods);

            fprintf(f,"%-*s" INT_FORMAT "\n", PARS_INDENT, "BREG:alg_index", reg_pars.b_rst_pars.alg_index);
            fprintf(f,"%-*s" INT_FORMAT "\n", PARS_INDENT, "BREG:dead_beat", reg_pars.b_rst_pars.dead_beat);

            fprintf(f,"%-*s" DOUBLE_FORMAT "\n", PARS_INDENT, "BREG:track_delay_periods",
                               reg_pars.b_rst_pars.track_delay_periods);
            fprintf(f,"%-*s" DOUBLE_FORMAT "\n", PARS_INDENT, "BREG:ref_delay_periods",
                               reg_pars.b_rst_pars.ref_delay_periods);

            for(i = 0 ; i < REG_N_RST_COEFFS ; i++)
            {
                fprintf(f,"%-*s" DOUBLE_FORMAT "  " DOUBLE_FORMAT "  " DOUBLE_FORMAT "\n", PARS_INDENT, "BREG:R:S:T",
                               reg_pars.b_rst_pars.rst.r[i],
                               reg_pars.b_rst_pars.rst.s[i],
                               reg_pars.b_rst_pars.rst.t[i]);
            }
            fprintf(f,"%-*s" DOUBLE_FORMAT "\n\n", PARS_INDENT, "BREG:t0_correction",
                               reg_pars.b_rst_pars.t0_correction);
        }

        // Report internally calculated current regulation parameters

        if(ccrun.ireg_flag == 1)
        {
            fprintf(f,"%-*s" DOUBLE_FORMAT "\n", PARS_INDENT, "IREG:pure_delay_periods",
                               reg_pars.i_rst_pars.pure_delay_periods);

            fprintf(f,"%-*s" INT_FORMAT "\n", PARS_INDENT, "IREG:alg_index", reg_pars.i_rst_pars.alg_index);
            fprintf(f,"%-*s" INT_FORMAT "\n", PARS_INDENT, "IREG:dead_beat", reg_pars.i_rst_pars.dead_beat);

            fprintf(f,"%-*s" DOUBLE_FORMAT "\n", PARS_INDENT, "IREG:track_delay_periods",
                               reg_pars.i_rst_pars.track_delay_periods);
            fprintf(f,"%-*s" DOUBLE_FORMAT "\n", PARS_INDENT, "IREG:ref_delay_periods",
                               reg_pars.i_rst_pars.ref_delay_periods);

            for(i = 0 ; i < REG_N_RST_COEFFS ; i++)
            {
                fprintf(f,"%-*s" FLOAT_FORMAT " " DOUBLE_FORMAT " " FLOAT_FORMAT " " DOUBLE_FORMAT
                             " " DOUBLE_FORMAT " " DOUBLE_FORMAT " " DOUBLE_FORMAT "\n",
                                 PARS_INDENT, "IREG:R:S:T:A:B:AS+BR:JURY",
                               reg_pars.i_rst_pars.rst.r[i],
                               reg_pars.i_rst_pars.rst.s[i],
                               reg_pars.i_rst_pars.rst.t[i],
                               reg_pars.i_rst_pars.a[i],
                               reg_pars.i_rst_pars.b[i],
                               reg_pars.i_rst_pars.asbr[i],
                               reg_pars.i_rst_pars.jury[i]);
            }
            fprintf(f,"%-*s" INT_FORMAT "\n", PARS_INDENT, "IREG:jurys_result", reg_pars.i_rst_pars.jurys_result);
            fprintf(f,"%-*s" DOUBLE_FORMAT " " DOUBLE_FORMAT "\n", PARS_INDENT, "IREG:sum_even_s:sum_odd_s",
                               reg_pars.i_rst_pars.sum_even_s, reg_pars.i_rst_pars.sum_odd_s);
            fprintf(f,"%-*s" DOUBLE_FORMAT "\n",   PARS_INDENT, "IREG:sum_even_s+sum_odd_s",
                               reg_pars.i_rst_pars.sum_even_s + reg_pars.i_rst_pars.sum_odd_s);
            fprintf(f,"%-*s" DOUBLE_FORMAT "\n",   PARS_INDENT, "IREG:sum_even_s-sum_odd_s",
                               reg_pars.i_rst_pars.sum_even_s - reg_pars.i_rst_pars.sum_odd_s);
            fprintf(f,"%-*s" DOUBLE_FORMAT "\n\n", PARS_INDENT, "IREG:t0_correction",
                               reg_pars.i_rst_pars.t0_correction);
        }
    }

    // Report function meta data from last run

    for(i = 0 ; i < ccrun.num_functions ; i++)
    {
        if(i > 0)
        {
            fputc('\n',f);
        }

        fprintf(f,"%-*s %s\n",   PARS_INDENT, "FUNCTION:",
                            ccParsEnumString(function_type, ccpars_global.function[i]));
        fprintf(f,"%-*s %s\n",   PARS_INDENT, "REG_MODE:",
                            ccParsEnumString(reg_mode,      ccpars_global.reg_mode[i]));
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "REG:ref_advance",     ccrun.ref_advance[i]);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "REG:max_abs_err",     ccrun.max_abs_err[i]);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "FG_META:duration",    ccrun.fg_meta[i].duration);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "FG_META:range.start", ccrun.fg_meta[i].range.start);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "FG_META:range.end",   ccrun.fg_meta[i].range.end);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "FG_META:range.min",   ccrun.fg_meta[i].range.min);
        fprintf(f,"%-*s" FLOAT_FORMAT "\n", PARS_INDENT, "FG_META:range.max",   ccrun.fg_meta[i].range.max);
    }
}
// EOF

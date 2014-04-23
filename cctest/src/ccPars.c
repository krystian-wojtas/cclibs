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
#include <errno.h>

// Include cctest program header files

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRef.h"
#include "ccRun.h"

/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccParsGet(char *cmd_name, struct ccpars *par, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to interpret arguments remaining on the input line as values belonging to
  a command parameter.
\*---------------------------------------------------------------------------------------------------------*/
{

    char                *remaining_arg;
    char                *arg;
    size_t               arg_len;
    struct ccpars_enum  *par_enum;
    struct ccpars_enum  *par_enum_matched;

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
                ccTestPrintError("invalid integer for %s %s[%u]: '%s'",
                        cmd_name, par->name, par->num_elements, arg);
                return(EXIT_FAILURE);
            }
            break;

        case PAR_FLOAT:

            par->value_p.f[par->num_elements] = strtod(arg, &remaining_arg);

            if(*remaining_arg != '\0' || errno != 0)
            {
                ccTestPrintError("invalid float for %s %s[%u]: '%s'",
                        cmd_name, par->name, par->num_elements, arg);
                return(EXIT_FAILURE);
            }
            break;

        case PAR_STRING:

            // On first call, allocate space for array of pointers to strings

            if(par->value_p.s == NULL)
            {
                par->value_p.s = calloc(par->max_num_elements, sizeof(char*));
            }

            // Free and reallocate space for string argment

            free(par->value_p.s[par->num_elements]);
            par->value_p.s[par->num_elements] = strcpy(malloc(arg_len),arg);
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

            fprintf(f,"% d",par->value_p.i[idx]);
            break;

        case PAR_FLOAT:

            fprintf(f,"% .6E",par->value_p.f[idx]);
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
/*---------------------------------------------------------------------------------------------------------*/
void ccParsPrintDebug(FILE *f)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t     i;

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        // Report internally calculated load parameters

        fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "LOAD:inv_henrys",
                     reg_pars.load_pars.inv_henrys);
        fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "LOAD:ohms",
                     reg_pars.load_pars.ohms);
        fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "LOAD:tc",
                     reg_pars.load_pars.tc);
        fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "LOAD:gain0",
                     reg_pars.load_pars.gain0);
        fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "LOAD:gain1",
                     reg_pars.load_pars.gain1);
        fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "LOAD:gain2",
                     reg_pars.load_pars.gain2);
        fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "LOAD:gain3",
                     reg_pars.load_pars.gain3);
        fprintf(f,"%-*s% .6E\n\n", PARS_INDENT, "LOAD:gain10",
                     reg_pars.load_pars.gain10);

        if(reg_pars.load_pars.sat.i_end > 0.0)
        {
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "LOAD:sat.i_delta",
                         reg_pars.load_pars.sat.i_delta);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "LOAD:sat.b_end",
                         reg_pars.load_pars.sat.b_end);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "LOAD:sat.b_factor",
                         reg_pars.load_pars.sat.b_factor);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "LOAD:sat.l_rate",
                         reg_pars.load_pars.sat.l_rate);
            fprintf(f,"%-*s% .6E\n\n", PARS_INDENT, "LOAD:sat.l_clip",
                         reg_pars.load_pars.sat.l_clip);
        }

        // Report internally calculated simulated load parameters

        fprintf(f,"%-*s% d\n",     PARS_INDENT, "SIMLOAD:vs_undersampled_flag",
                     reg_pars.sim_load_pars.vs_undersampled_flag);
        fprintf(f,"%-*s% d\n",     PARS_INDENT, "SIMLOAD:load_undersampled_flag",
                     reg_pars.sim_load_pars.load_undersampled_flag);
        fprintf(f,"%-*s% .6E\n\n", PARS_INDENT, "SIMLOAD:period_tc_ratio",
                     reg_pars.sim_load_pars.period_tc_ratio);

        if(ccpars_load.sim_tc_error != 0.0)
        {
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:ohms_ser",
                         reg_pars.sim_load_pars.load_pars.ohms_ser);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:ohms_par",
                         reg_pars.sim_load_pars.load_pars.ohms_par);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:ohms_mag",
                         reg_pars.sim_load_pars.load_pars.ohms_mag);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:henrys",
                         reg_pars.sim_load_pars.load_pars.henrys);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:inv_henrys",
                         reg_pars.sim_load_pars.load_pars.inv_henrys);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:ohms",
                         reg_pars.sim_load_pars.load_pars.ohms);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:tc",
                         reg_pars.sim_load_pars.load_pars.tc);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:gain0",
                         reg_pars.sim_load_pars.load_pars.gain0);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:gain1",
                         reg_pars.sim_load_pars.load_pars.gain1);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:gain2",
                         reg_pars.sim_load_pars.load_pars.gain2);
            fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:gain3",
                         reg_pars.sim_load_pars.load_pars.gain3);
            fprintf(f,"%-*s% .6E\n\n", PARS_INDENT, "SIMLOAD:gain10",
                         reg_pars.sim_load_pars.load_pars.gain10);

            if(reg_pars.sim_load_pars.load_pars.sat.i_end > 0.0)
            {
                fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:sat.henrys",
                             reg_pars.sim_load_pars.load_pars.sat.henrys);
                fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:sat.i_delta",
                             reg_pars.sim_load_pars.load_pars.sat.i_delta);
                fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:sat.b_end",
                             reg_pars.sim_load_pars.load_pars.sat.b_end);
                fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:sat.b_factor",
                             reg_pars.sim_load_pars.load_pars.sat.b_factor);
                fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:sat.l_rate",
                             reg_pars.sim_load_pars.load_pars.sat.l_rate);
                fprintf(f,"%-*s% .6E\n\n", PARS_INDENT, "SIMLOAD:sat.l_clip",
                             reg_pars.sim_load_pars.load_pars.sat.l_clip);
            }
        }

        // Report internally calculated voltage source parameters

        fprintf(f,"%-*s% .6E,% .6E,% .6E,% .6E\n", PARS_INDENT, "SIMVS:numerator",
                     reg_pars.sim_vs_pars.num[0],
                     reg_pars.sim_vs_pars.num[1],
                     reg_pars.sim_vs_pars.num[2],
                     reg_pars.sim_vs_pars.num[3]);

        fprintf(f,"%-*s% .6E,% .6E,% .6E,% .6E\n", PARS_INDENT, "SIMVS:denominator",
                     reg_pars.sim_vs_pars.den[0],
                     reg_pars.sim_vs_pars.den[1],
                     reg_pars.sim_vs_pars.den[2],
                     reg_pars.sim_vs_pars.den[3]);

        fprintf(f,"%-*s% .6E\n",   PARS_INDENT, "SIMVS:step_rsp_time_iters",reg_pars.sim_vs_pars.step_rsp_time_iters);
        fprintf(f,"%-*s% .6E\n\n", PARS_INDENT, "SIMVS:gain",               reg_pars.sim_vs_pars.gain);

        // Report internally calculated field regulation parameters

        if(ccrun.breg_flag == 1)
        {
            fprintf(f,"%-*s% 16.9E\n", PARS_INDENT, "B_RST:pure_delay_periods",
                               reg_pars.b_rst_pars.pure_delay_periods);

            fprintf(f,"%-*s% d\n", PARS_INDENT, "B_RST:alg_index", reg_pars.b_rst_pars.alg_index);
            fprintf(f,"%-*s% d\n", PARS_INDENT, "B_RST:dead_beat", reg_pars.b_rst_pars.dead_beat);

            fprintf(f,"%-*s% 16.9E\n", PARS_INDENT, "B_RST:track_delay_periods",
                               reg_pars.b_rst_pars.track_delay_periods);

            for(i = 0 ; i < REG_N_RST_COEFFS ; i++)
            {
                fprintf(f,"%-*s% 16.9E  % 16.9E  % 16.9E\n", PARS_INDENT, "B_RST:",
                               reg_pars.b_rst_pars.rst.r[i],
                               reg_pars.b_rst_pars.rst.s[i],
                               reg_pars.b_rst_pars.rst.t[i]);
            }
            fprintf(f,"%-*s% 16.9E\n\n", PARS_INDENT, "B_RST:t0_correction",
                               reg_pars.b_rst_pars.t0_correction);
        }

        // Report internally calculated current regulation parameters

        if(ccrun.ireg_flag == 1)
        {
            fprintf(f,"%-*s% 16.9E\n", PARS_INDENT, "I_RST:pure_delay_periods",
                               reg_pars.i_rst_pars.pure_delay_periods);

            fprintf(f,"%-*s% d\n", PARS_INDENT, "I_RST:alg_index", reg_pars.i_rst_pars.alg_index);
            fprintf(f,"%-*s% d\n", PARS_INDENT, "I_RST:dead_beat", reg_pars.i_rst_pars.dead_beat);

            fprintf(f,"%-*s% 16.9E\n", PARS_INDENT, "I_RST:track_delay_periods",
                               reg_pars.i_rst_pars.track_delay_periods);

            for(i = 0 ; i < REG_N_RST_COEFFS ; i++)
            {
                fprintf(f,"%-*s% 16.9E  % 16.9E  % 16.9E\n", PARS_INDENT, "I_RST:",
                               reg_pars.i_rst_pars.rst.r[i],
                               reg_pars.i_rst_pars.rst.s[i],
                               reg_pars.i_rst_pars.rst.t[i]);
            }
            fprintf(f,"%-*s% 16.9E\n\n", PARS_INDENT, "I_RST:t0_correction",
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
        fprintf(f,"%-*s% .6E\n", PARS_INDENT, "REG:ref_advance",     ccrun.ref_advance[i]);
        fprintf(f,"%-*s% .6E\n", PARS_INDENT, "FG_META:duration",    ccrun.fg_meta[i].duration);
        fprintf(f,"%-*s% .6E\n", PARS_INDENT, "FG_META:range.start", ccrun.fg_meta[i].range.start);
        fprintf(f,"%-*s% .6E\n", PARS_INDENT, "FG_META:range.end",   ccrun.fg_meta[i].range.end);
        fprintf(f,"%-*s% .6E\n", PARS_INDENT, "FG_META:range.min",   ccrun.fg_meta[i].range.min);
        fprintf(f,"%-*s% .6E\n", PARS_INDENT, "FG_META:range.max",   ccrun.fg_meta[i].range.max);
    }
}
// EOF

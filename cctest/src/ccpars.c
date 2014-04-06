/*---------------------------------------------------------------------------------------------------------*\
  File:     ccpars.c                                                                    Copyright CERN 2014

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

#include "ccpars.h"
#include "ccref.h"
#include "ccsigs.h"
#include "ccrun.h"

/*---------------------------------------------------------------------------------------------------------*/
static void ccparsGetPar(char *line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to interpret one line of input.  Blank lines and comment (#) lines are ignored.
  All other lines must have the format: GROUP.PARAMETER VALUE(S)
  Where VALUES are comma separated.
\*---------------------------------------------------------------------------------------------------------*/
{
    char                remains;
    char                *ch_p;
    char                par_string[PARS_MAX_FILE_LINE_LEN];
    struct ccpars_group *group;
    struct ccpars       *par;
    struct ccpars_enum  *par_enum;

    // Check if line exceeds maximum length

    if(strlen(line) >= (PARS_MAX_FILE_LINE_LEN-1))
    {
        line[20] = '\0';
        fprintf(stderr,"Error: Line starting \"%s...\" is too long (%u max)\n",
                       line,PARS_MAX_FILE_LINE_LEN-2);
        exit(EXIT_FAILURE);
    }

    // Skip leading white space

    ch_p = line;

    while(isspace(*ch_p))
    {
        ch_p++;
    }

    // Skip blank lines and comment lines

    if(*ch_p == '\0' || *ch_p == '#')
    {
        return;
    }

    // Try to identify the parameter group name

    ch_p = strtok( ch_p, ".\n" );

    for(group = ccpars_groups ; group->name != NULL && strcasecmp(group->name,ch_p) != 0; group++);

    if(group->name == NULL)
    {
        ch_p[20] = '\0';       // Protect against long strings
        fprintf(stderr,"Error: Unknown parameter group: \"%s\"\n", ch_p);
        exit(EXIT_FAILURE);
    }

    // Try to identify the parameter name within the group

    ch_p = strtok( NULL, " \t\n" );

    for(par = group->pars ; par->name != NULL && strcasecmp(par->name,ch_p) != 0; par++);

    if(par->name == NULL)
    {
        ch_p[20] = '\0';       // Protect against long strings
        fprintf(stderr,"Error: Unknown parameter: \"%s.%s\"\n",group->name,ch_p);
        exit(EXIT_FAILURE);
    }

    // Try to parse values for the parameter

    par->num_values = 0;

    while((ch_p = strtok( NULL, ",\n" )) != NULL)
    {
        if(par->num_values >= par->max_values)
        {
            fprintf(stderr,"Error: Too many values for %s.%s (%u max)\n",
                    group->name,par->name,par->max_values);
            exit(EXIT_FAILURE);
        }

        switch(par->type)
        {
        case PAR_UNSIGNED:

            if(sscanf(ch_p," %u %c",&par->value_p.i[par->num_values],&remains) != 1)
            {
                fprintf(stderr,"Error: Invalid integer for %s.%s: %s\n",
                        group->name,par->name,ch_p);
                exit(EXIT_FAILURE);
            }
            break;

        case PAR_FLOAT:

            if(sscanf(ch_p," %e %c",&par->value_p.f[par->num_values],&remains) != 1)
            {
                fprintf(stderr,"Error: Invalid float for %s.%s: %s\n",
                        group->name,par->name,ch_p);
                exit(EXIT_FAILURE);
            }
            break;

        case PAR_STRING:

            if(sscanf(ch_p," %s %c",par_string,&remains) != 1)
            {
                fprintf(stderr,"Error: Invalid token for %s.%s: %s\n",
                        group->name,par->name,ch_p);
                exit(EXIT_FAILURE);
            }

            par->value_p.s[par->num_values] = strcpy(malloc(strlen(par_string)),par_string);
            break;

        case PAR_ENUM:

            if(sscanf(ch_p," %s %c",par_string,&remains) != 1)
            {
                ch_p[30] = '\0';       // Protect against long strings
                fprintf(stderr,"Error: Invalid token for %s.%s: %s\n",
                        group->name,par->name,ch_p);
                exit(EXIT_FAILURE);
            }

            for(par_enum = par->ccpars_enum ;
                par_enum->string != NULL && strcasecmp(par_enum->string,par_string) != 0 ;
                par_enum++);

            if(par_enum->string == NULL)
            {
                ch_p[30] = '\0';       // Protect against long strings
                fprintf(stderr,"Error: Unknown value for %s.%s: %s\n",
                        group->name,par->name,par_string);
                exit(EXIT_FAILURE);
            }

            par->value_p.i[par->num_values] = par_enum->value;
            break;
        }

        par->num_values++;
    }

    // Increase parameters read for each group - this can count the same parameter multiple times

    group->n_pars_read++;
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccparsPrintf(char * format, ...)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print to memory that will later be written to the output to create the parameter and
  debug pop-ups
\*---------------------------------------------------------------------------------------------------------*/
{
    va_list     argv;
    int         n_chars;
    char        print_buf[PARS_MAX_PRINT_LINE_LEN];

    // Check for overflow of the report line pointer buffer

    if(ccpars_report.num_lines > PARS_MAX_REPORT_LINES)
    {
        fprintf(stderr,"Error: Max number of report lines (%d) exceeded\n",PARS_MAX_REPORT_LINES);
        exit(EXIT_FAILURE);
    }

    // Print report line to local buffer variable

    va_start(argv, format);
    n_chars = vsnprintf(print_buf, PARS_MAX_PRINT_LINE_LEN, format, argv);
    va_end(argv);

    // Allocate memory for line, line to report line pointer array and copy line

    free(ccpars_report.line_buf[ccpars_report.num_lines]);

    ccpars_report.line_buf[ccpars_report.num_lines] = (char *)calloc(n_chars+1, sizeof(char));

    strcpy(ccpars_report.line_buf[ccpars_report.num_lines++], print_buf);
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccparsReportPars(char * group_name, struct ccpars *par)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print the parameter values
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t            idx;
    uint32_t            n_chars;
    uint32_t            num_values;
    char                par_name_buf[PARS_INDENT+1];
    char                value_buf[PARS_MAX_PRINT_LINE_LEN];

    while(par->name != NULL)
    {
        snprintf(par_name_buf,PARS_INDENT,"%s.%s",group_name,par->name);

        num_values = (par->num_values > 0 ? par->num_values : par->default_values);

        for( n_chars = idx = 0 ; idx < num_values ; idx ++)
        {
            if(idx > 0)
            {
                value_buf[n_chars++] = ',';
            }

            switch(par->type)
            {
            case PAR_UNSIGNED:

                n_chars += sprintf(&value_buf[n_chars],"% d",par->value_p.i[idx]);
                break;

            case PAR_FLOAT:

                n_chars += sprintf(&value_buf[n_chars],"% .6E",par->value_p.f[idx]);
                break;

            case PAR_STRING:

                n_chars += sprintf(&value_buf[n_chars]," %s",par->value_p.s[idx]);
                break;

            case PAR_ENUM:

                n_chars += sprintf(&value_buf[n_chars]," %s",ccparsEnumString(par->ccpars_enum, par->value_p.i[idx]));
                break;
            }
        }
        ccparsPrintf("%-*s%s\n", PARS_INDENT, par_name_buf,value_buf);
        par++;
    }

    ccparsPrintf("\n");
}
/*---------------------------------------------------------------------------------------------------------*/
char * ccparsEnumString(struct ccpars_enum *par_enum, uint32_t value)
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
static uint32_t ccparsCheckMissingPars(enum ccpars_groups_enum group_idx)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct ccpars_group *group = &ccpars_groups[group_idx];
    struct ccpars       *par;

    // If any parameters of a required group are missing, report the details

    if(group->n_pars_missing > 0)
    {
        fprintf(stderr,"Error: Group %s requires all parameters to be fully defined:\n",group->name);

        for(par = group->pars; par->name != NULL ; par++)
        {
            if(par->num_values < par->min_values)
            {
                fprintf(stderr,"    %s.%s - %u of %u supplied\n",
                        group->name,par->name,par->min_values,par->num_values);
            }
        }
        return(1);  // Return error code
    }

    group->enabled = 1;     // Mark group as enabled
    return(0);              // Return no error
}
/*---------------------------------------------------------------------------------------------------------*/
void ccparsGet(int argc, char **argv)
/*---------------------------------------------------------------------------------------------------------*/
{
    char    line[PARS_MAX_FILE_LINE_LEN];
    struct ccpars_group *group;
    struct ccpars       *par;

    // Process all lines from standard input

    while(fgets(line, PARS_MAX_FILE_LINE_LEN, stdin) != NULL)
    {
        ccparsGetPar(line);
    }

    // Process all parameters from arguments

    while(--argc > 0)
    {
        ccparsGetPar(*(++argv));
    }

    // Count how many parameters are missing values

    for(group = ccpars_groups ; group->name != NULL ; group++)
    {
        group->enabled = 0;

        for(par = group->pars ; par->name != NULL ; par++)
        {
            if(par->num_values < par->min_values)
            {
                group->n_pars_missing++;
            }
        }
    }

    // Global group must always be valid

    ccparsCheckMissingPars(GROUP_GLOBAL);

    // Function data must be defined for the specified function

    ccparsCheckMissingPars(func[ccpars_global.function].group_idx);

    // if GLOBAL.REVERSE_TIME is enabled then GLOBAL.FG_LIMITS and GLOBAL.SIM_LOAD must be DISABLED

    if(ccpars_global.reverse_time == CC_ENABLED)
    {
        if(ccpars_global.sim_load == CC_ENABLED || ccpars_global.fg_limits == CC_ENABLED)
        {
            fputs("Error: When GLOBAL.REVERSE_TIME is ENABLED, GLOBAL.FG_LIMITS and GLOBAL.SIM_LOAD must be DISABLED\n",stderr);
            exit(EXIT_FAILURE);
        }
    }

    // if GLOBAL.REG_MODE is FIELD or CURRENT then SIM_LOAD must be ENABLED

    if(ccpars_global.reg_mode != REG_VOLTAGE && ccpars_global.sim_load == CC_DISABLED)
    {
        fputs("Error: GLOBAL.REG_MODE of FIELD or CURRENT requires GLOBAL.SIM_LOAD to be ENABLED\n",stderr);
        exit(EXIT_FAILURE);
    }

    // LIMITS group required if GLOBAL.FG_LIMITS or GLOBAL.SIM_LOAD enabled

    if(ccpars_global.fg_limits == CC_ENABLED || ccpars_global.sim_load == CC_ENABLED)
    {
        ccparsCheckMissingPars(GROUP_LIMITS);
    }

    // LOAD and VS parameters requirements depend on SIM_LOAD

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        ccparsCheckMissingPars(GROUP_LOAD);
        ccparsCheckMissingPars(GROUP_VS);

        // If voltage perturbation is not required then set perturb_time to far beyond end of simulation

        if(ccpars_load.perturb_time <= 0.0 || ccpars_load.perturb_volts == 0.0)
        {
            ccpars_load.perturb_volts = 0.0;
            ccpars_load.perturb_time  = 1.0E30;
        }

        // Ensure that open loop time/duration are coherent

        if(ccpars_global.reg_mode == REG_VOLTAGE || ccpars_global.open_loop_time <= 0.0 || ccpars_global.open_loop_duration <= 0.0)
        {
            ccpars_global.open_loop_time = 1.0E30;
        }
        else
        {
            ccrun.close_loop_time = ccpars_global.open_loop_time + ccpars_global.open_loop_duration;
        }

        // Ensure that VS.V_REF_DELAY_ITERS is at least 1

        if(ccpars_vs.v_ref_delay_iters < 1.0)
        {
            fprintf(stderr,"Error: VS.V_REF_DELAY_ITERS (%g) must be >= 1.0\n",ccpars_vs.v_ref_delay_iters);
            exit(EXIT_FAILURE);
        }
    }

    // Regulation parameters requirement depends on reg mode

    if(ccpars_global.reg_mode == REG_FIELD)
    {
        ccparsCheckMissingPars(GROUP_REG_B);
    }

    if(ccpars_global.reg_mode == REG_CURRENT)
    {
        ccparsCheckMissingPars(GROUP_REG_I);
    }

    // If simulation is enabled then ensure that open loop time/duration are coherent and v_ref_delay_iters >= 1

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        if(ccpars_global.reg_mode == REG_VOLTAGE || ccpars_global.open_loop_time <= 0.0 || ccpars_global.open_loop_duration <= 0.0)
        {
            ccpars_global.open_loop_time = 1.0E30;
        }
        else
        {
            ccrun.close_loop_time = ccpars_global.open_loop_time + ccpars_global.open_loop_duration;
        }

        if(ccpars_vs.v_ref_delay_iters < 1.0)
        {
            fprintf(stderr,"Error: VS.V_REF_DELAY_ITERS (%g) must be >= 1.0\n",ccpars_vs.v_ref_delay_iters);
            exit(EXIT_FAILURE);
        }
    }

    // START function and ABORT_TIME require units of AMPS or GAUSS and FG_LIMITS ENABLED

    if(ccpars_global.function == FG_START || ccpars_global.abort_time > 0.0)
    {
        if(ccpars_global.reg_mode == REG_VOLTAGE)
        {
            fputs("Error: START function and GLOBAL.ABORT_TIME require GLOBAL.REG_MODE of FIELD or CURRENT\n",stderr);
            exit(EXIT_FAILURE);
        }

        if(ccpars_global.fg_limits == CC_DISABLED)
        {
            fputs("Error: START function and GLOBAL.ABORT_TIME require GLOBAL.FG_LIMITS to be ENABLED\n",stderr);
            exit(EXIT_FAILURE);
        }
    }

    // ABORT_TIME cannot lie within the open loop time window

    if(ccpars_global.abort_time > 0.0)
    {
        if(ccpars_global.abort_time <= ccrun.close_loop_time)
        {
            fprintf(stderr,"Error: ABORT_TIME (%.6f) must not be before the end of the open loop window (%.6f)\n",
                        ccpars_global.abort_time, ccrun.close_loop_time);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        ccpars_global.abort_time = 1.0E30;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccparsGenerateReport(void)
/*---------------------------------------------------------------------------------------------------------*/
{
    unsigned             i;
    struct ccpars_group *group = ccpars_groups;

    ccpars_report.num_lines = 0;

    // For FLOT output, generate inline div for the Show pars pop-up

    if(ccpars_global.output_format == CC_FLOT)
    {
        ccparsPrintf("<!-- Simulation parameters pop-up -->\n\n");
        ccparsPrintf("    <div id='inline_pars' style='padding:10px; background:#fff;font-size:14px;'>\n");
        ccparsPrintf("      <p style='font-size:22px;font-weight:bold;'>cctest Simulation Parameters:</p>\n      <p><pre>\n");
    }

    // Generate report of parameter values if GLOBAL.VERBOSE is enabled or FLOT output format selected

    if(ccpars_global.verbose == CC_ENABLED || ccpars_global.output_format == CC_FLOT)
    {
        while(group->name)
        {
            if(group->enabled == 1)
            {
                ccparsReportPars(group->name,group->pars);
            }

            group++;
        }
    }

    // For FLOT output, generate inline div for the Show debug pop-up

    if(ccpars_global.output_format == CC_FLOT)
    {
        ccparsPrintf("      </pre></p>\n    </div>\n\n<!-- Debug parameters pop-up -->\n\n");
        ccparsPrintf("    <div id='inline_debug' style='padding:10px; background:#fff;font-size:14px;'>\n");
        ccparsPrintf("      <p style='font-size:22px;font-weight:bold;'>cctest Debug Information:</p>\n      <p><pre>\n");
    }

    if(ccpars_groups[GROUP_LOAD].enabled == 1)
    {
        // Report internally calculated load parameters

        ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:inv_henrys",
                     reg_pars.load_pars.inv_henrys);
        ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:ohms",
                     reg_pars.load_pars.ohms);
        ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:tc",
                     reg_pars.load_pars.tc);
        ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:gain0",
                     reg_pars.load_pars.gain0);
        ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:gain1",
                     reg_pars.load_pars.gain1);
        ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:gain2",
                     reg_pars.load_pars.gain2);
        ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:gain3",
                     reg_pars.load_pars.gain3);
        ccparsPrintf("%-*s% .6E\n\n", PARS_INDENT, "LOAD:gain10",
                     reg_pars.load_pars.gain10);

        if(reg_pars.load_pars.sat.i_end > 0.0)
        {
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:sat.i_delta",
                         reg_pars.load_pars.sat.i_delta);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:sat.b_end",
                         reg_pars.load_pars.sat.b_end);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:sat.b_factor",
                         reg_pars.load_pars.sat.b_factor);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:sat.l_rate",
                         reg_pars.load_pars.sat.l_rate);
            ccparsPrintf("%-*s% .6E\n\n", PARS_INDENT, "LOAD:sat.l_clip",
                         reg_pars.load_pars.sat.l_clip);
        }

        // Report internally calculated simulated load parameters

        ccparsPrintf("%-*s% d\n",     PARS_INDENT, "SIMLOAD:vs_undersampled_flag",
                     reg_pars.sim_load_pars.vs_undersampled_flag);
        ccparsPrintf("%-*s% d\n",     PARS_INDENT, "SIMLOAD:load_undersampled_flag",
                     reg_pars.sim_load_pars.load_undersampled_flag);
        ccparsPrintf("%-*s% .6E\n\n", PARS_INDENT, "SIMLOAD:period_tc_ratio",
                     reg_pars.sim_load_pars.period_tc_ratio);

        if(ccpars_load.sim_tc_error != 0.0)
        {
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:ohms_ser",
                         reg_pars.sim_load_pars.load_pars.ohms_ser);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:ohms_par",
                         reg_pars.sim_load_pars.load_pars.ohms_par);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:ohms_mag",
                         reg_pars.sim_load_pars.load_pars.ohms_mag);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:henrys",
                         reg_pars.sim_load_pars.load_pars.henrys);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:inv_henrys",
                         reg_pars.sim_load_pars.load_pars.inv_henrys);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:ohms",
                         reg_pars.sim_load_pars.load_pars.ohms);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:tc",
                         reg_pars.sim_load_pars.load_pars.tc);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:gain0",
                         reg_pars.sim_load_pars.load_pars.gain0);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:gain1",
                         reg_pars.sim_load_pars.load_pars.gain1);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:gain2",
                         reg_pars.sim_load_pars.load_pars.gain2);
            ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:gain3",
                         reg_pars.sim_load_pars.load_pars.gain3);
            ccparsPrintf("%-*s% .6E\n\n", PARS_INDENT, "SIMLOAD:gain10",
                         reg_pars.sim_load_pars.load_pars.gain10);

            if(reg_pars.sim_load_pars.load_pars.sat.i_end > 0.0)
            {
                ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:sat.henrys",
                             reg_pars.sim_load_pars.load_pars.sat.henrys);
                ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:sat.i_delta",
                             reg_pars.sim_load_pars.load_pars.sat.i_delta);
                ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:sat.b_end",
                             reg_pars.sim_load_pars.load_pars.sat.b_end);
                ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:sat.b_factor",
                             reg_pars.sim_load_pars.load_pars.sat.b_factor);
                ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMLOAD:sat.l_rate",
                             reg_pars.sim_load_pars.load_pars.sat.l_rate);
                ccparsPrintf("%-*s% .6E\n\n", PARS_INDENT, "SIMLOAD:sat.l_clip",
                             reg_pars.sim_load_pars.load_pars.sat.l_clip);
            }
        }
    }

    if(ccpars_groups[GROUP_VS].enabled == 1)
    {
        // Report internally calculated voltage source parameters

        ccparsPrintf("%-*s% .6E,% .6E,% .6E,% .6E\n", PARS_INDENT, "SIMVS:numerator",
                     reg_pars.sim_vs_pars.num[0],
                     reg_pars.sim_vs_pars.num[1],
                     reg_pars.sim_vs_pars.num[2],
                     reg_pars.sim_vs_pars.num[3]);

        ccparsPrintf("%-*s% .6E,% .6E,% .6E,% .6E\n", PARS_INDENT, "SIMVS:denominator",
                     reg_pars.sim_vs_pars.den[0],
                     reg_pars.sim_vs_pars.den[1],
                     reg_pars.sim_vs_pars.den[2],
                     reg_pars.sim_vs_pars.den[3]);

        ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "SIMVS:step_rsp_time_iters",reg_pars.sim_vs_pars.step_rsp_time_iters);
        ccparsPrintf("%-*s% .6E\n\n", PARS_INDENT, "SIMVS:gain",               reg_pars.sim_vs_pars.gain);
    }

    if(reg.mode == REG_FIELD)
    {
        // Report internally calculated field regulation parameters

        ccparsPrintf("%-*s% d\n", PARS_INDENT, "B_RST:alg_index", reg_pars.b_rst_pars.alg_index);
        ccparsPrintf("%-*s% d\n", PARS_INDENT, "B_RST:dead_beat", reg_pars.b_rst_pars.dead_beat);

        for(i = 0 ; i < REG_N_RST_COEFFS ; i++)
        {
            ccparsPrintf("%-*s% 16.9E  % 16.9E  % 16.9E\n", PARS_INDENT, "B_RST:",
                           reg_pars.b_rst_pars.rst.r[i],
                           reg_pars.b_rst_pars.rst.s[i],
                           reg_pars.b_rst_pars.rst.t[i]);
        }
        ccparsPrintf("%-*s% 16.9E\n",   PARS_INDENT, "B_RST:track_delay_periods",
                           reg_pars.b_rst_pars.track_delay_periods);
        ccparsPrintf("%-*s% 16.9E\n\n", PARS_INDENT, "B_RST:t0_correction",
                           reg_pars.b_rst_pars.t0_correction);
    }

    if(reg.mode == REG_CURRENT)
    {
        // Report internally calculated field regulation parameters

        ccparsPrintf("%-*s% d\n", PARS_INDENT, "I_RST:alg_index", reg_pars.i_rst_pars.alg_index);
        ccparsPrintf("%-*s% d\n", PARS_INDENT, "I_RST:dead_beat", reg_pars.i_rst_pars.dead_beat);

        for(i = 0 ; i < REG_N_RST_COEFFS ; i++)
        {
            ccparsPrintf("%-*s% 16.9E  % 16.9E  % 16.9E\n", PARS_INDENT, "I_RST:",
                           reg_pars.i_rst_pars.rst.r[i],
                           reg_pars.i_rst_pars.rst.s[i],
                           reg_pars.i_rst_pars.rst.t[i]);
        }
        ccparsPrintf("%-*s% 16.9E\n",   PARS_INDENT, "I_RST:track_delay_periods",
                           reg_pars.i_rst_pars.track_delay_periods);
        ccparsPrintf("%-*s% 16.9E\n\n", PARS_INDENT, "I_RST:t0_correction",
                           reg_pars.i_rst_pars.t0_correction);
    }

    // Report ref advance

    if(reg.mode != REG_VOLTAGE)
    {
        ccparsPrintf("%-*s% .6E\n", PARS_INDENT, "REG.ref_advance", reg.ref_advance);
    }

    // Report function meta data

    ccparsPrintf("%-*s% .6E\n", PARS_INDENT, "FG_META:duration",    fg_meta.duration);
    ccparsPrintf("%-*s% .6E\n", PARS_INDENT, "FG_META:range.start", fg_meta.range.start);
    ccparsPrintf("%-*s% .6E\n", PARS_INDENT, "FG_META:range.end",   fg_meta.range.end);
    ccparsPrintf("%-*s% .6E\n", PARS_INDENT, "FG_META:range.min",   fg_meta.range.min);
    ccparsPrintf("%-*s% .6E\n", PARS_INDENT, "FG_META:range.max",   fg_meta.range.max);

    // Print report to stderr if GLOBAL.VERBOSE is enabled

    if(ccpars_global.verbose == CC_ENABLED)
    {
        ccparsPrintReport(stderr);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccparsPrintReport(FILE *f)
/*---------------------------------------------------------------------------------------------------------*/
{
    unsigned        i;

    for(i = 0 ; i < ccpars_report.num_lines ; i++)
    {
        fputs(ccpars_report.line_buf[i],f);
    }
}
// EOF

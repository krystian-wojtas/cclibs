/*---------------------------------------------------------------------------------------------------------*\
  File:     ccpars.c                                                                    Copyright CERN 2011

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

  Purpose:  Fg/Reg library test program parameter file parsing functions

  Author:   Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

// Include libfg and libreg header files

#include "libfg/plep.h"
#include "libfg/ramp.h"
#include "libfg/pppl.h"
#include "libfg/spline.h"
#include "libfg/table.h"
#include "libfg/test.h"
#include "libfg/trim.h"
#include "libreg.h"

// Include cctest parameter header files

#include "pars/global.h"
#include "pars/limits.h"
#include "pars/load.h"
#include "pars/reg.h"
#include "pars/vs.h"

// Include cctest function data header files

#include "func/start.h"
#include "func/plep.h"
#include "func/ramp.h"
#include "func/pppl.h"
#include "func/table.h"
#include "func/trim.h"
#include "func/test.h"

// Include cctest program header files

#include "ccref.h"
#include "ccsigs.h"
#include "ccrun.h"

// External for command line options (this is a gcc library extension)

extern char *optarg;

/*---------------------------------------------------------------------------------------------------------*/
static void ccparsReadFile(char option, struct ccpars *ccpars, char *filename, uint32_t *status)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to read the parameters from the specified filename.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t            line_number;
    FILE                *f;
    char                remains;
    char                *ch_p;
    char                line[PARS_MAX_FILE_LINE_LEN];
    char                par_string[PARS_MAX_FILE_LINE_LEN];
    struct ccpars       *par;
    struct ccpars_enum  *par_enum;

    // Check if this command line option file was already read

    if(*status != 0)
    {
        fprintf(stderr,"Error : option -%c already processed\n",option);
        exit(EXIT_FAILURE);
    }

    // Open parameter file

    if((f = fopen(filename, "rt")) == NULL)
    {
        fprintf(stderr,"Error : Failed to open %s: %s\n",filename,strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Read parameter file

    line_number = 0;

    while(fgets(line, PARS_MAX_FILE_LINE_LEN, f) != NULL)
    {
        line_number++;

        if(strlen(line) >= (PARS_MAX_FILE_LINE_LEN-1))
        {
            fprintf(stderr,"Error in %s: Line %u is too long (%u max)\n",
                           filename,line_number,PARS_MAX_FILE_LINE_LEN-2);
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
            continue;
        }

        // Try to identify the parameter name

        ch_p = strtok( ch_p, " \t\n" );

        for(par = ccpars ; par->name != NULL && strcasecmp(par->name,ch_p) != 0; par++);

        if(par->name == NULL)
        {
            fprintf(stderr,"Error in %s at line %u: Unknown parameter: \"%s\"\n",filename,line_number,ch_p);
            exit(EXIT_FAILURE);
        }

        // Try to parse values

        while((ch_p = strtok( NULL, ",\n" )) != NULL)
        {
            if(par->num_values >= par->max_values)
            {
                fprintf(stderr,"Error in %s at line %u: Too many values for %s (%u max)\n",
                        filename,line_number,par->name,par->max_values);
                exit(EXIT_FAILURE);
            }

            switch(par->type)
            {
            case PAR_UNSIGNED:

                if(sscanf(ch_p," %u %c",&par->value_p.i[par->num_values],&remains) != 1)
                {
                    fprintf(stderr,"Error in %s at line %u: Invalid integer for %s: \"%s\"\n",
                            filename,line_number,par->name,ch_p);
                    exit(EXIT_FAILURE);
                }
                break;

            case PAR_FLOAT:

                if(sscanf(ch_p," %e %c",&par->value_p.f[par->num_values],&remains) != 1)
                {
                    fprintf(stderr,"Error in %s at line %u: Invalid float for %s: %s\n",
                            filename,line_number,par->name,ch_p);
                    exit(EXIT_FAILURE);
                }
                break;

            case PAR_STRING:

                if(sscanf(ch_p," %s %c",par_string,&remains) != 1)
                {
                    fprintf(stderr,"Error in %s at line %u: Invalid token for %s: %s\n",
                            filename,line_number,par->name,ch_p);
                    exit(EXIT_FAILURE);
                }

                par->value_p.s[par->num_values] = strcpy(malloc(strlen(par_string)),par_string);
                break;

            case PAR_ENUM:

                if(sscanf(ch_p," %s %c",par_string,&remains) != 1)
                {
                    fprintf(stderr,"Error in %s at line %u: Invalid token for %s: %s\n",
                            filename,line_number,par->name,ch_p);
                    exit(EXIT_FAILURE);
                }

                for(par_enum = par->ccpars_enum ; par_enum->string != NULL && strcasecmp(par_enum->string,par_string) != 0 ; par_enum++);

                if(par_enum->string == NULL)
                {
                    fprintf(stderr,"Error in %s at line %u: Unknown value for %s: %s\n",
                            filename,line_number,par->name,par_string);
                    exit(EXIT_FAILURE);
                }

                par->value_p.i[par->num_values] = par_enum->value;
                break;
            }

            par->num_values++;
        }
    }

    // Close parameter file

    if(fclose(f))
    {
        fprintf(stderr,"Error : Failed to close %s: %s\n",filename,strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Check if any parameters values are missing

    for(par = ccpars ; par->name ; par++)
    {
        if(par->num_values < par->min_values)
        {
            fprintf(stderr,"Error in %s: parameter %s: %u value%s required, %u read\n",
                    filename,par->name,par->min_values,(par->min_values != 1 ? "s" : ""),par->num_values);

            exit(EXIT_FAILURE);
        }
    }

    // Flag that this parameter group has been read

    *status = 1;
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
        fprintf(stderr,"Error: max number of report lines (%d) exceeded\n",PARS_MAX_REPORT_LINES);
        exit(EXIT_FAILURE);
    }

    // Print report line to local buffer variable

    va_start(argv, format);
    n_chars = vsnprintf(print_buf, PARS_MAX_PRINT_LINE_LEN, format, argv);
    va_end(argv);

    // Allocate memory for line, line to report line pointer array and copy line

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
static void ccparsOutputFormat(char *arg)
/*---------------------------------------------------------------------------------------------------------*/
{
    struct ccpars_enum  *par_enum;

    for(par_enum = output_format ; par_enum->string != NULL && strcasecmp(par_enum->string,arg) != 0 ; par_enum++);

    if(par_enum->string == NULL)
    {
        fprintf(stderr,"Error: %s is not a known output format\n",arg);
        exit(EXIT_FAILURE);
    }

    ccpars_global.output_format_opt = par_enum->value;
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
void ccparsGet(int argc, char **argv)
/*---------------------------------------------------------------------------------------------------------*/
{
    int c;

    // Process command-line options using getopt (this is a gcc library extension)

    while((c = getopt(argc, argv, "vo:g:f:d:m:l:s:r:")) != -1)
    {
        switch(c)
        {
        default:

            printf("\nUsage: %s [-v] [-o<OUTPUT_FORMAT>] -g<FILE> [-f FUNC] -d<FILE> [-m<FILE>] [-l<FILE>] [-s<FILE>] [-r<FILE>]\n\n%s\n", argv[0],
                   "         -v                      Verbose: report all parameter values\n"
                   "         -o<OUTPUT FORMAT>       STANDARD|FGCSPY|LVDV|FLOT\n"
                   "         -g<GLOBAL PARS FILE>    Global parameters file\n"
                   "         -f<FUNCTION>            Function type\n"
                   "         -d<FUNCTION DATA FILE>  Function data file\n"
                   "         -m<LIMIT PARS FILE>     Limit parameters file\n"
                   "         -l<LOAD PARS FILE>      Load model parameters file\n"
                   "         -s<VS PARS FILE>        Voltage source model parameters file\n"
                   "         -r<REG PARS FILE>       Regulation parameters file\n"
                  );
            exit(EXIT_FAILURE);

        case 'v':           // Enable verbose mode

            ccpars_global.verbose_flag = 1;
            break;

        case 'o':           // Output format

            ccparsOutputFormat(optarg);
            break;

        case 'g':           // Global parameters file

            ccparsReadFile(c, global_pars_list, optarg, &ccpars_global.status);
            break;

        case 'm':           // Limit parameters file

            ccparsReadFile(c, limit_pars_list, optarg, &ccpars_limits.status);
            break;

        case 'l':           // Load parameters file

            ccparsReadFile(c, load_pars_list, optarg, &ccpars_load.status);
            break;

        case 's':           // Voltage source parameters file

            ccparsReadFile(c, vs_pars_list, optarg, &ccpars_vs.status);
            break;

        case 'r':           // Regulation parameters file

            ccparsReadFile(c, reg_pars_list, optarg, &ccpars_reg.status);
            break;

        case 'f':           // Function type

            ccrefFuncType(optarg);
            break;

        case 'd':           // Function data file

            if(ccpars_global.status == 0)
            {
                fputs("Error : global parameters (-g) must precede function data (-d)\n",stderr);
                exit(EXIT_FAILURE);
            }

            if(ccpars_global.function == FG_NONE)
            {
                fputs("Error : function type must be specifed in gobal parameters file (-g) or as an option (-f)\n",stderr);
                exit(EXIT_FAILURE);
            }

            ccparsReadFile(c, func[ccpars_global.function].pars, optarg, &ccpars_global.func_data_status);
            break;
        }
    }

    // Check parameters are all coherent

    // Output format (-o) takes presidence over GLOBAL.OUTPUT_FORMAT parameter if specified

    if(ccpars_global.output_format_opt != FG_NONE)
    {
        ccpars_global.output_format = ccpars_global.output_format_opt;
    }

    // Function data (-d) must always be defined (which requires global parameters to be defined)

    if(ccpars_global.func_data_status == 0)
    {
        fputs("Error : function data (-d) must be specified\n",stderr);
        exit(EXIT_FAILURE);
    }

    // REVERSE_TIME: FG_LIMITS and SIM_LOAD must be DISABLED

    if(ccpars_global.reverse_time == CC_ENABLED)
    {
        if(ccpars_global.sim_load == CC_ENABLED || ccpars_global.fg_limits == CC_ENABLED)
        {
            fputs("Error : With REVERSE_TIME: FG_LIMITS and SIM_LOAD must be DISABLED\n",stderr);
            exit(EXIT_FAILURE);
        }
    }

    // UNITS of AMPS or GAUSS requires SIM_LOAD to be ENABLED

    if(ccpars_global.units != REG_VOLTAGE && ccpars_global.sim_load == CC_DISABLED)
    {
        fputs("Error : UNITS of AMPS or GAUSS requires SIM_LOAD to be ENABLED\n",stderr);
        exit(EXIT_FAILURE);
    }

    // Limit parameters (-m) requirement depends on FG_LIMITS and SIM_LOAD

    if(ccpars_global.fg_limits == CC_DISABLED && ccpars_global.sim_load == CC_DISABLED)
    {
        if(ccpars_limits.status == 1)
        {
            fputs("Error : Limits parameters (-m) not required\n",stderr);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if(ccpars_limits.status == 0)
        {
            fputs("Error : Limits parameters (-m) must be provided\n",stderr);
            exit(EXIT_FAILURE);
        }
    }

    // Load (-l) and voltage source (-s) parameters requirements depend on SIM_LOAD

    if(ccpars_global.sim_load == CC_DISABLED)
    {
        if(ccpars_load.status == 1)
        {
            fputs("Error : Load parameters (-l) are not required\n",stderr);
            exit(EXIT_FAILURE);
        }

        if(ccpars_vs.status == 1)
        {
            fputs("Error : SIM_LOAD is DISABLED so voltage source parameters (-s) are not required\n",stderr);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if(ccpars_load.status == 0)
        {
            fputs("Error : Load parameters (-l) required\n",stderr);
            exit(EXIT_FAILURE);
        }

        // SIM_LOAD ENABLED : Always enable VS parameters since default values are supplied

        ccpars_vs.status = 1;

        // If voltage perturbation is not required then set perturb_time to far beyond end of simulation

        if(ccpars_load.perturb_time <= 0.0 || ccpars_load.perturb_volts == 0.0)
        {
            ccpars_load.perturb_time = 1.0E30;
        }
    }

    // Regulation parameters (-r) requirement depends on UNITS

    if(ccpars_global.units != REG_VOLTAGE)
    {
        if(ccpars_reg.status == 0)
        {
            fputs("Error : Regulation parameters (-r) must be provided\n",stderr);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if(ccpars_reg.status == 1)
        {
            fputs("Error : Regulation parameters (-r) not required\n",stderr);
            exit(EXIT_FAILURE);
        }
    }

    // If simulation is enabled then ensure that open loop time/duration are coherent and v_ref_delay >= 1

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        if(ccpars_global.units == REG_VOLTAGE || ccpars_reg.ol_time <= 0.0 || ccpars_reg.ol_duration <= 0.0)
        {
            ccpars_reg.ol_time = 1.0E30;
        }
        else
        {
            ccpars_reg.cl_time = ccpars_reg.ol_time + ccpars_reg.ol_duration;
        }

        if(ccpars_vs.v_ref_delay < ccpars_global.iter_period)
        {
            fprintf(stderr,"Error : VS.V_REF_DELAY (%g) must be >= GLOBAL.ITER_PERIOD (%g)\n",
                    ccpars_vs.v_ref_delay, ccpars_global.iter_period);
            exit(EXIT_FAILURE);
        }
    }

    // START function and ABORT_TIME require units of AMPS or GAUSS and FG_LIMITS ENABLED

    if(ccpars_global.function == FG_START || ccpars_global.abort_time > 0.0)
    {
        if(ccpars_global.units == REG_VOLTAGE)
        {
            fputs("Error : START function and ABORT_TIME require units of AMPS or GAUSS\n",stderr);
            exit(EXIT_FAILURE);
        }

        if(ccpars_global.fg_limits == CC_DISABLED)
        {
            fputs("Error : START function and ABORT_TIME require FG_LIMITS to be ENABLED\n",stderr);
            exit(EXIT_FAILURE);
        }
    }

    // ABORT_TIME cannot lie within the open loop time window

    if(ccpars_global.abort_time > 0.0)
    {
        if(ccpars_global.abort_time <= ccpars_reg.cl_time)
        {
            fprintf(stderr,"Error : ABORT_TIME (%.6f) must not be before the end of the open loop window (%.6f)\n",
                        ccpars_global.abort_time, ccpars_reg.cl_time);
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
    unsigned        i;

    // Generate inline div for the Show pars pop-up

    ccparsPrintf("<!-- Simulation parameters pop-up -->\n\n");
    ccparsPrintf("    <div id='inline_pars' style='padding:10px; background:#fff;font-size:14px;'>\n");
    ccparsPrintf("      <p style='font-size:22px;font-weight:bold;'>cctest Simulation Parameters:</p>\n      <p><pre>\n");

    // Generate report of parameter values if verbose option (-v) or FLOT output format selected

    if(ccpars_global.verbose_flag || ccpars_global.output_format == CC_FLOT)
    {
        ccparsReportPars("GLOBAL",global_pars_list);

        if(ccpars_limits.status == 1)
        {
            ccparsReportPars("LIMITS",limit_pars_list);
        }

        if(ccpars_load.status == 1)
        {
            ccparsReportPars("LOAD",load_pars_list);
        }

        if(ccpars_vs.status == 1)
        {
            ccparsReportPars("VS",vs_pars_list);
        }

        if(ccpars_reg.status == 1)
        {
            ccparsReportPars("REG",reg_pars_list);
        }

        ccparsReportPars("DATA",func[ccpars_global.function].pars);
    }

    // Generate inline div for the Show debug pop-up

    ccparsPrintf("      </pre></p>\n    </div>\n\n<!-- Debug parameters pop-up -->\n\n");
    ccparsPrintf("    <div id='inline_debug' style='padding:10px; background:#fff;font-size:14px;'>\n");
    ccparsPrintf("      <p style='font-size:22px;font-weight:bold;'>cctest Debug Information:</p>\n      <p><pre>\n");

    if(ccpars_load.status == 1)
    {
        // Report measurement filter information

        ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:i_meas.num0_correction",
                     reg_pars.i_meas.num0_correction);
        ccparsPrintf("%-*s% u\n",     PARS_INDENT, "LOAD:i_meas.order",
                     reg_pars.i_meas.order);
        ccparsPrintf("%-*s% .6E\n",   PARS_INDENT, "LOAD:b_meas.num0_correction",
                     reg_pars.b_meas.num0_correction);
        ccparsPrintf("%-*s% u\n\n",   PARS_INDENT, "LOAD:b_meas.order",
                     reg_pars.b_meas.order);

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

    if(ccpars_vs.status == 1)
    {
        // Report voltage measurement filter information

        ccparsPrintf("%-*s% .6E\n", PARS_INDENT, "VS:v_meas.num0_correction",
                     reg_pars.v_meas.num0_correction);
        ccparsPrintf("%-*s% u\n\n",   PARS_INDENT, "VS:v_meas.order",
                     reg_pars.v_meas.order);

        // Report internally calculated parameters

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

        ccparsPrintf("%-*s% .6E\n\n", PARS_INDENT, "SIMVS:gain",reg_pars.sim_vs_pars.gain);
    }

    if(ccpars_reg.status == 1)
    {
        if(ccpars_global.units == REG_CURRENT)
        {
            for(i = 0 ; i < REG_N_RST_COEFFS ; i++)
            {
               ccparsPrintf("%-*s% 16.9E  % 16.9E  % 16.9E\n", PARS_INDENT, "RST:",
                               reg_pars.i_rst_pars.rst.r[i],
                               reg_pars.i_rst_pars.rst.s[i],
                               reg_pars.i_rst_pars.rst.t[i]);
            }
            ccparsPrintf("%-*s% 16.9E\n\n", PARS_INDENT, "RST:t0_correction",
                               reg_pars.i_rst_pars.t0_correction);
        }
        else
        {
            for(i = 0 ; i < REG_N_RST_COEFFS ; i++)
            {
               ccparsPrintf("%-*s% 16.9E  % 16.9E  % 16.9E\n", PARS_INDENT, "RST:",
                               reg_pars.b_rst_pars.rst.r[i],
                               reg_pars.b_rst_pars.rst.s[i],
                               reg_pars.b_rst_pars.rst.t[i]);
            }
            ccparsPrintf("%-*s% 16.9E\n\n", PARS_INDENT, "RST:t0_correction",
                               reg_pars.b_rst_pars.t0_correction);
        }
    }

    ccparsPrintf("%-*s% .6E\n", PARS_INDENT, "FG_META:duration",    fg_meta.duration);
    ccparsPrintf("%-*s% .6E\n", PARS_INDENT, "FG_META:range.start", fg_meta.range.start);
    ccparsPrintf("%-*s% .6E\n", PARS_INDENT, "FG_META:range.end",   fg_meta.range.end);
    ccparsPrintf("%-*s% .6E\n", PARS_INDENT, "FG_META:range.min",   fg_meta.range.min);
    ccparsPrintf("%-*s% .6E\n", PARS_INDENT, "FG_META:range.max",   fg_meta.range.max);

    // Print report to stderr if verbose flag (-v) was set

    if(ccpars_global.verbose_flag)
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
/*---------------------------------------------------------------------------------------------------------*\
  End of file: ccpars.c
\*---------------------------------------------------------------------------------------------------------*/


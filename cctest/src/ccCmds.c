/*---------------------------------------------------------------------------------------------------------*\
  File:     ccCmds.c                                                                    Copyright CERN 2014

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
#include "ccSigs.h"
#include "ccInit.h"
#include "ccRun.h"

/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsHelp(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print a help message listing all the commands
\*---------------------------------------------------------------------------------------------------------*/
{
    if(ccTestNoMoreArgs(remaining_line) == EXIT_FAILURE)
    {
        return(EXIT_FAILURE);
    }

    for(cmd_idx = 0 ; cmd_idx < N_CMDS ; cmd_idx++)
    {
        printf("%-*s %s\n", CC_MAX_CMD_NAME_LEN, cmds[cmd_idx].name, cmds[cmd_idx].help_message);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsCd(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to set the current directory using the supplied parameter
\*---------------------------------------------------------------------------------------------------------*/
{
    char   *arg;
    FILE   *f;
    char   *wd;
    char    cwd_buf[CC_PATH_LEN];

    arg = ccTestGetArgument(remaining_line);

    // If no argument supplied then use the base path for cctest project

    if(arg == NULL)
    {
        arg = cctest.base_path;
    }
    else if(ccTestNoMoreArgs(remaining_line) == EXIT_FAILURE)
    {
        return(EXIT_FAILURE);
    }

    // If changing directory fails, then report the error message

    if(chdir(arg) != EXIT_SUCCESS)
    {
        ccTestPrintError("changing directory to '%s' : %s (%d)",
                          ccTestAbbreviatedArg(arg), strerror(errno), errno);
        return(EXIT_FAILURE);
    }

    // If reading from stdin, open CMD file to save the current working directory

    if(cctest.cwd_file_path[0] != '\0')
    {
        f = fopen(cctest.cwd_file_path,"w");

        if(f == NULL)
        {
            printf("Fatal - failed to open '%s' : %s (%d)\n", cctest.cwd_file_path, strerror(errno), errno);
            exit(EXIT_FAILURE);
        }

        // Read the current working directory and write to the file

        wd = getcwd(cwd_buf, sizeof(cwd_buf));

        if(wd == NULL)
        {
            ccTestPrintError("getting current directory : %s (%d)", strerror(errno), errno);
            return(EXIT_FAILURE);
        }

        fputs(wd,f);

        fclose(f);
    }

    // Print new current working directory

    return(ccCmdsPwd(0, remaining_line));
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsPwd(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will display the current directory
\*---------------------------------------------------------------------------------------------------------*/
{
    char    cwd_buf[CC_PATH_LEN];
    char    *wd;

    // No arguments expected

    if(ccTestNoMoreArgs(remaining_line) == EXIT_FAILURE)
    {
        return(EXIT_FAILURE);
    }

    // Get and print current working directory

    wd = getcwd(cwd_buf, sizeof(cwd_buf));

    if(wd == NULL)
    {
        ccTestPrintError("getting current directory : %s (%d)", strerror(errno), errno);
        return(EXIT_FAILURE);
    }

    puts(wd);

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsLs(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will display the contents of the current directory using the ls command.  It will supply
  any arguments provided to ls.
\*---------------------------------------------------------------------------------------------------------*/
{
    char ls_command[CC_MAX_FILE_LINE_LEN];

    // Construct ls command

    strcpy(ls_command, "ls ");

    if(*remaining_line != NULL)
    {
        strcat(ls_command, *remaining_line);
    }

    // Use system to run ls command

    if(system(ls_command) != 0)
    {
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsRead(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to read lines from stdin or from file named in the supplied parameter. It can be
  called recursively via the call to ccTestParseLine(). It protects against nesting open files too deeply.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t       exit_status = EXIT_SUCCESS;
    char           line[CC_MAX_FILE_LINE_LEN];
    char           input_ch;
    FILE          *f;
    char          *arg;
    static char *  default_file_name = "cctest";

    // Check that input file nesting limit has not been exceeded

    if(cctest.input_idx >= (CC_INPUT_FILE_NEST_LIMIT - 1))
    {
        ccTestPrintError("input file nesting limit (%u) reached", CC_INPUT_FILE_NEST_LIMIT);
        return(EXIT_FAILURE);
    }

    // Get the first argument (if provided) and check that there are no more arguments after that

    arg = ccTestGetArgument(remaining_line);

    if(ccTestNoMoreArgs(remaining_line))
    {
        return(EXIT_FAILURE);
    }

    // If no argument supplied then default to reading stdin

    if(arg == NULL)
    {
        // If already reading from stdin or from a file then report an error

        if(cctest.input_idx > 0)
        {
            ccTestPrintError("already reading from a file or from stdin");
            return(EXIT_FAILURE);
        }

        // Read from stdin

        f = stdin;
        cctest.input_idx++;
        cctest.input[cctest.input_idx].line_number = 0;
        cctest.input[cctest.input_idx].path        = default_file_name;

        // Try to recover and display saved working directory

        ccTestRecoverPath();
        ccCmdsPwd(0, remaining_line);
        printf(CC_PROMPT);
    }
    else
    {
        // If argument is "*" then read all files in current working directory

        if(strcmp(arg, "*") == 0)
        {
            return(ccTestReadAllFiles());
        }

        // Try to open named file

        f = fopen(arg, "r");

        if(f == NULL)
        {
             ccTestPrintError("opening file '%s' : %s (%d)", ccTestAbbreviatedArg(arg), strerror(errno), errno);
             return(EXIT_FAILURE);
        }

        printf("Reading parameters from '%s'\n", arg);

        // Stack new file information

        cctest.input_idx++;
        cctest.input[cctest.input_idx].line_number = 1;
        cctest.input[cctest.input_idx].path        = arg;
    }

    // Process all lines from the new file or from stdin

    while(fgets(line, CC_MAX_FILE_LINE_LEN, f) != NULL)
    {
        // Check if line was too long (2 chars are needed for newline and terminating nul)

        if(strlen(line) >= (CC_MAX_FILE_LINE_LEN-1) && line[CC_MAX_FILE_LINE_LEN-2] != '\n')
        {
            ccTestPrintError("line exceeds maximum length (%u)", CC_MAX_FILE_LINE_LEN-2);
            exit_status = EXIT_FAILURE;

            // Purge the rest of the line

            while((input_ch = fgetc(f)) != '\n' && input_ch != EOF);
        }
        else
        {
            // Parse the input line

            exit_status = ccTestParseLine(line);
        }

        // Print prompt when using stdin

        if(f == stdin)
        {
            printf(CC_PROMPT);
        }
        else // else when reading from file, break out if error reported and stop on error is enabled
        {
            if(ccpars_global.stop_on_error == CC_ENABLED && exit_status == EXIT_FAILURE)
            {
                break;
            }

            cctest.input[cctest.input_idx].line_number++;
        }
    }

    // If reading from a file then close it

    if(f != stdin)
    {
        fclose(f);
        cctest.input_idx--;
    }

    return(exit_status);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsSave(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will save all the parameters to file
\*---------------------------------------------------------------------------------------------------------*/
{
    FILE *f;
    char *arg;
    char *default_filename = "cctest_pars";
    struct cccmds *cmd;

    arg = ccTestGetArgument(remaining_line);

    // If no argument supplied then use the default filename

    if(arg == NULL)
    {
        arg = default_filename;
    }
    else if(ccTestNoMoreArgs(remaining_line) == EXIT_FAILURE)
    {
        return(EXIT_FAILURE);
    }

    // Try to open named file to write

    f = fopen(arg, "w");

    if(f == NULL)
    {
         ccTestPrintError("opening file '%s' : %s (%d)", ccTestAbbreviatedArg(arg), strerror(errno), errno);
         return(EXIT_FAILURE);
    }

    printf("Writing all parameters to '%s'...",arg);

    fprintf(f,"# CCTEST v%.2f\n",CC_VERSION);

    for(cmd = cmds ; cmd->name != NULL ; cmd++)
    {
        if(cmd->pars != NULL)
        {
            fprintf(f, "\n# %s Parameters\n\n", cmd->name);

            ccParsPrintAll(f, cmd->name, cmd->pars);
        }
    }

    fputs("\n# EOF\n",f);
    fclose(f);
    puts("done.");
    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsDebug(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will display the debug information for all the active parameters from the previous run.
\*---------------------------------------------------------------------------------------------------------*/
{
    // No arguments expected

    if(ccTestNoMoreArgs(remaining_line))
    {
        return(EXIT_FAILURE);
    }

    // Print debug information from previous run to stdout

    ccParsPrintDebug(stdout);

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsRun(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will launch a run of the function generation and optionally simulate the voltage source
  and load.
\*---------------------------------------------------------------------------------------------------------*/
{
    char *filename;

    // No arguments expected

    if(ccTestNoMoreArgs(remaining_line))
    {
        return(EXIT_FAILURE);
    }

    // Initialise run, the load model and the reference functions

    if(ccInitRun()       == EXIT_FAILURE ||
       ccInitLoad()      == EXIT_FAILURE ||
       ccInitFunctions() == EXIT_FAILURE)
    {
        return(EXIT_FAILURE);
    }

    // If the load will be simulated, initialise the limits, regulation and simulation

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        if(ccInitLimits()     == EXIT_FAILURE ||
           ccInitSimulation() == EXIT_FAILURE)
        {
            return(EXIT_FAILURE);
        }
        // Initialise regulation - must be after ccInitSimulation()

        if((ccrun.breg_flag == 1 && ccInitRegulation(&ccpars_breg, &conv.b, REG_FIELD,   "FIELD"  ) == EXIT_FAILURE) ||
           (ccrun.ireg_flag == 1 && ccInitRegulation(&ccpars_ireg, &conv.i, REG_CURRENT, "CURRENT") == EXIT_FAILURE))
        {
            // If STOP_ON_ERROR is DISABLED then dump debug data automatically to stdout

            if(ccpars_global.stop_on_error == CC_DISABLED)
            {
                ccParsPrintDebug(stdout);
            }

            return(EXIT_FAILURE);
        }
    }

    // Open CSV output file

    filename = strcmp(ccpars_global.file, "*") != 0 ? ccpars_global.file : cctest.input[cctest.input_idx].path;

    if(ccpars_global.csv_format != CC_NONE)
    {
        char     csv_path[CC_PATH_LEN];
        char     csv_filename[CC_PATH_LEN];

        snprintf(csv_path, CC_PATH_LEN, "%s/results/csv/%s/%s",
                 cctest.base_path,
                 ccpars_global.group,
                 ccpars_global.project);

        if(ccTestMakePath(csv_path) == EXIT_FAILURE)
        {
            return(EXIT_FAILURE);
        }

        snprintf(csv_filename, CC_PATH_LEN, "%s/%s.csv", csv_path, filename);

        cctest.csv_file = fopen(csv_filename, "w");

        if(cctest.csv_file == NULL)
        {
             ccTestPrintError("opening file '%s' : %s (%d)", csv_filename, strerror(errno), errno);
             return(EXIT_FAILURE);
        }
    }

    // Enable signals that are to be logged

    ccSigsInit();

    // Run the test

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        // Generate functions and simulate voltage source and load and regulate if required

        printf("Running simulation to %s/%s/%s\n", ccpars_global.group, ccpars_global.project, filename);

        ccRunSimulation();
    }
    else
    {
        // Generate reference function only - no load simulation: this is just to test libfg functions

        if(ccpars_global.reverse_time == CC_DISABLED)
        {
            printf("Generating function(s) to %s/%s/%s\n",
                    ccpars_global.group, ccpars_global.project, filename);

            ccRunFuncGen();
        }
        else // Reverse time can be used with only one function
        {
            printf("Generating function with reverse time to %s/%s/%s\n",
                    ccpars_global.group, ccpars_global.project, filename);

            ccRunFuncGenReverseTime();
        }
    }

    // Close CSV output file

    if(ccpars_global.csv_format != CC_NONE)
    {
        fclose(cctest.csv_file);
    }

    // Write FLOT data if required

    if(ccpars_global.flot_output == CC_ENABLED)
    {
        FILE    *flot_file;
        char     flot_path[CC_PATH_LEN];
        char     flot_filename[CC_PATH_LEN];

        snprintf(flot_path, CC_PATH_LEN, "%s/results/webplots/%s/%s",
                 cctest.base_path,
                 ccpars_global.group,
                 ccpars_global.project);

        if(ccTestMakePath(flot_path) == EXIT_FAILURE)
        {
            return(EXIT_FAILURE);
        }

        snprintf(flot_filename, CC_PATH_LEN, "%s/%s.html", flot_path, filename);

        flot_file = fopen(flot_filename, "w");

        if(flot_file == NULL)
        {
             ccTestPrintError("opening file '%s' : %s (%d)", flot_filename, strerror(errno), errno);
             return(EXIT_FAILURE);
        }

        ccSigsFlot(flot_file, filename);

        fclose(flot_file);
    }

    // Write Debug file if required

    if(ccpars_global.debug_output == CC_ENABLED)
    {
        FILE    *debug_file;
        char     debug_path[CC_PATH_LEN];
        char     debug_filename[CC_PATH_LEN];
        struct cccmds *cmd;

        snprintf(debug_path, CC_PATH_LEN, "%s/results/debug/%s/%s",
                 cctest.base_path,
                 ccpars_global.group,
                 ccpars_global.project);

        if(ccTestMakePath(debug_path) == EXIT_FAILURE)
        {
            return(EXIT_FAILURE);
        }

        snprintf(debug_filename, CC_PATH_LEN, "%s/%s.ccd", debug_path, filename);

        debug_file = fopen(debug_filename, "w");

        if(debug_file == NULL)
        {
             ccTestPrintError("opening file '%s' : %s (%d)", debug_filename, strerror(errno), errno);
             return(EXIT_FAILURE);
        }

        // Print debug data to debug file

        ccParsPrintDebug(debug_file);

        // Print parameters to debug file

        for(cmd = cmds ; cmd->name != NULL ; cmd++)
        {
            fputc('\n', debug_file);

            if(cmd->enabled == 1)
            {
                ccParsPrintAll(debug_file, cmd->name, cmd->pars);
            }
        }

        fclose(debug_file);
    }

    // Report bad values that were sent to ccSigsStore()

    return(ccSigsReportBadValues());
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsPar(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print or set parameters
\*---------------------------------------------------------------------------------------------------------*/
{
    char               *arg;
    uint32_t            arg_len;
    struct ccpars      *par_matched;
    struct ccpars      *par;
    struct ccpars_enum *par_enum;

    arg = ccTestGetArgument(remaining_line);

    // If no arguments then print the names and value(s) for all the parameter for this command

    if(arg == NULL)
    {
        ccParsPrintAll(stdout, cmds[cmd_idx].name, cmds[cmd_idx].pars);
    }
    else
    {
        arg_len = strlen(arg);

        // Compare first argument against list of parameters to find unambiguous match

        for(par = cmds[cmd_idx].pars, par_matched = NULL ; par->name != NULL ; par++)
        {
             // If command argument matches start or all of a command

            if(strncasecmp(par->name, arg, arg_len) == 0)
            {
                // If argument exactly matches a parameter name then take use it

                if(strlen(par->name) == arg_len)
                {
                    par_matched = par;
                    break;
                }

                // If first partial match, remember command index

                if(par_matched == NULL)
                {
                    par_matched = par;
                }
                else // else second partial match so report error
                {
                    ccTestPrintError("ambiguous %s parameter '%s'", cmds[cmd_idx].name, arg);
                    return(EXIT_FAILURE);
                }
            }
        }

        if(par_matched == NULL)
        {
            ccTestPrintError("unknown parameter for %s: '%s'",
                             cmds[cmd_idx].name, ccTestAbbreviatedArg(arg));
            return(EXIT_FAILURE);
        }

        // If no arguments provided, report information about the parameter

        if(*remaining_line == NULL)
        {
            ccParsPrint(stdout, cmds[cmd_idx].name, par_matched);
            printf("Number of elements defined: %u\n",par_matched->num_elements);
            printf("Minimum number of elements: %u\n",par_matched->min_num_elements);
            printf("Maximum number of elements: %u\n",par_matched->max_num_elements);

            if(par_matched->type == PAR_ENUM)
            {
                printf("Range:");

                for(par_enum = par_matched->ccpars_enum ; par_enum->string != NULL ; par_enum++)
                {
                    printf(" %s",par_enum->string);
                }

                putchar('\n');
            }
        }
        else // else get parameter values from arguments
        {
            return(ccParsGet(cmds[cmd_idx].name, par_matched, remaining_line));
        }
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsExit(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will stop reading from the open file or stop the program if reading from stdin
\*---------------------------------------------------------------------------------------------------------*/
{
    if(ccTestNoMoreArgs(remaining_line) == EXIT_FAILURE)
    {
        return(EXIT_FAILURE);
    }

    // If processing commands from the command line or stdin then quit immediately

    if(cctest.input_idx == 0 || cctest.input[cctest.input_idx].line_number == 0)
    {
        ccCmdsQuit(0, remaining_line);
    }

    // Return failure to close current file

    return(EXIT_FAILURE);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsQuit(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will stop execution of cctest immediately
\*---------------------------------------------------------------------------------------------------------*/
{
    if(ccTestNoMoreArgs(remaining_line) == EXIT_FAILURE)
    {
        return(EXIT_FAILURE);
    }

    puts("Stopping cctest.");
    exit(EXIT_SUCCESS);
}
// EOF

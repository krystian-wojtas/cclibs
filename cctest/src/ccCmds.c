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
    char *arg;

    arg = ccTestGetArgument(remaining_line);

    if(ccTestNoMoreArgs(remaining_line) == EXIT_FAILURE)
    {
        return(EXIT_FAILURE);
    }

    if(arg != NULL && chdir(arg) != 0)
    {
        ccTestPrintError("changing directory to '%s' : %s (%d)\n",
                          ccTestAbbreviatedArg(arg), strerror(errno), errno);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsPwd(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will display the current directory using the supplied parameter
\*---------------------------------------------------------------------------------------------------------*/
{
    char    cwd_buf[CC_PATH_LEN];
    char    *wd;

    if(ccTestNoMoreArgs(remaining_line) == EXIT_FAILURE)
    {
        return(EXIT_FAILURE);
    }

    wd = getcwd(cwd_buf, sizeof(cwd_buf));

    if(wd == NULL)
    {
        ccTestPrintError("getting current directory : %s (%d)\n", strerror(errno), errno);
        return(EXIT_FAILURE);
    }

    puts(wd);

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsLs(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will display the current directory using the supplied parameter
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
        f = stdin;
        cctest.input_idx++;
        cctest.input[cctest.input_idx].line_number = 0;

        // Display working directory and prompt

        ccCmdsPwd(0, remaining_line);
        printf(CC_PROMPT);
    }
    else
    {
        // Try to open named file

        f = fopen(arg, "r");

        if(f == NULL)
        {
             ccTestPrintError("opening file '%s' : %s (%d)", ccTestAbbreviatedArg(arg), strerror(errno), errno);
             return(EXIT_FAILURE);
        }

        // Stack new file information

        cctest.input_idx++;
        cctest.input[cctest.input_idx].line_number = 1;
        cctest.input[cctest.input_idx].path = arg;

        printf("Reading from '%s'\n", arg);
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
        else // else when reading from file, break out if error reported
        {
            if(exit_status == EXIT_FAILURE)
            {
                break;
            }

            cctest.input[cctest.input_idx].line_number++;
        }
    }

    // If reading a file then close it

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
  This function will
\*---------------------------------------------------------------------------------------------------------*/
{

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsDebug(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will display the debug information for all the active parameters.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Reset report
/*
    ccpars_report.num_lines = 0;
    ccparsGenerateDebugReport();
    ccparsPrintReport(stdout);
*/
    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsRun(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will
\*---------------------------------------------------------------------------------------------------------*/
{/*
    // Initialise iteration period

    reg.iter_period = ccpars_global.iter_period;

    // Initialise load model (must be before ccInitFunction() if FG_LIMITS is enabled)

    ccInitLoad();

    // Initialise function to be generated

    ccInitFunction();

    // Initialise limits for the measurement, reference and regulation error if required

    ccInitLimits();

    // Initialise to simulate load and voltage source if required

    ccInitSimulation();

    // Initialise to regulate field or current if required

    ccInitRegulation();

    // Generate FLOT report of parameter values, if required

    ccParsGenerateFlotReport();

    // Enable signals to be written to stdout

    ccsigsInit();

    // Run the test

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        // Generate function and simulate voltage source and load and regulate if required

        printf("Running simulation... ");

        ccRunSimulation();
    }
    else
    {
        // Generate reference function only - no load simulation: this is just to test libfg functions

        printf("Generating function(s)... ");

        ccRunFuncGen();
    }

    // Write FLOT data if required

    ccSigsFlot();

    // Report completion of task

    printf("done.\n");
*/
    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsPar(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will
\*---------------------------------------------------------------------------------------------------------*/
{
    if(ccTestNoMoreArgs(remaining_line) == EXIT_FAILURE)
    {
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccCmdsExit(uint32_t cmd_idx, char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will stop execution of cctest
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

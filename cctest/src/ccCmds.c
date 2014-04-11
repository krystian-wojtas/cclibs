/*---------------------------------------------------------------------------------------------------------*\
  File:     cccmds.c                                                                    Copyright CERN 2014

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
#include "ccRef.h"
#include "ccSigs.h"
#include "ccRun.h"

/*---------------------------------------------------------------------------------------------------------*/
uint32_t cccmdsHelp(uint32_t cmd_idx, char *pars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print a help message listing all the commands
\*---------------------------------------------------------------------------------------------------------*/
{
    for(cmd_idx = 0 ; cmd_idx < N_CMDS ; cmd_idx++)
    {
        printf("%6s %s\n", cmds[cmd_idx].name, cmds[cmd_idx].help_message);
    }

    return(0);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t cccmdsCd(uint32_t cmd_idx, char *pars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to set the current directory using the supplied parameter
\*---------------------------------------------------------------------------------------------------------*/
{
    if(chdir(pars) != 0)
    {
        printf("Error changing directory : %s (%d)\n", strerror(errno), errno);
        return(1);
    }

    return(0);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t cccmdsPwd(uint32_t cmd_idx, char *pars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will display the current directory using the supplied parameter
\*---------------------------------------------------------------------------------------------------------*/
{
    char    cwd_buf[256];
    char    *wd;

    wd = getcwd(cwd_buf, sizeof(cwd_buf));

    if(wd == NULL)
    {
        printf("Error getting current directory : %s (%d)\n", strerror(errno), errno);
        return(1);
    }

    puts(wd);

    return(0);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t cccmdsRead(uint32_t cmd_idx, char *pars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to read lines from stdin or from file named in the supplied parameter
\*---------------------------------------------------------------------------------------------------------*/
{
    char           line[CC_MAX_FILE_LINE_LEN];
    struct ccpars *par;
    FILE          *f;

    // If pars equals "stdin" then use stdin stream

    if(strcmp(pars, "stdin") == 0)
    {
        f = stdin;

        // Display working directory and prompt

        cccmdsPwd(0, NULL);
        printf(CC_PROMPT);
    }
    else
    {
        // Try to open named file

        f = fopen(pars,"r");

        if(f == NULL)
        {
            printf("Error opening file <%s> : %s (%d)\n", pars, strerror(errno), errno);
            return(1);
        }

        printf("Reading %s\n",pars);
    }

    // Process all lines from file or stdin

    while(fgets(line, CC_MAX_FILE_LINE_LEN, f) != NULL)
    {
        // Parse the input line

        ccparsParseLine(line);

        // Print prompt when using stdin

        if(f == stdin)
        {
            printf(CC_PROMPT);
        }
    }

    return(0);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t cccmdsSave(uint32_t cmd_idx, char *pars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will
\*---------------------------------------------------------------------------------------------------------*/
{

    return(0);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t cccmdsDebug(uint32_t cmd_idx, char *pars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will display the debug information for all the active parameters.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Reset report

    ccpars_report.num_lines = 0;
    ccparsGenerateDebugReport();
    ccparsPrintReport(stdout);
    return(0);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t cccmdsRun(uint32_t cmd_idx, char *pars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will
\*---------------------------------------------------------------------------------------------------------*/
{
    // Initialise iteration period

    reg.iter_period = ccpars_global.iter_period;

    // Prepare load model (must be before PrepareFunction() if FG_LIMITS is enabled)

    PrepareLoad();

    // Prepare function to be generated

    PrepareFunction();

    // Prepare limits for the measurement, reference and regulation error if required

    PrepareLimits();

    // Prepare to simulate load and voltage source if required

    PrepareSimulation();

    // Prepare to regulate field or current if required

    PrepareRegulation();

    // Generate FLOT report of parameter values, if required

    ccparsGenerateFlotReport();

    // Enable signals to be written to stdout

    ccsigsPrepare();

    // Run the test

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        // Generate function and simulate voltage source and load and regulate if required

        printf("Running simulation... ");

        ccrunSimulation();
    }
    else
    {
        // Generate reference function only - no load simulation: this is just to test libfg functions

        printf("Generating function(s)... ");

        ccrunFunGen();
    }

    // Write FLOT data if required

    ccsigsFlot();

    // Report completion of task

    printf("done.\n");

    return(0);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t cccmdsPrint(uint32_t cmd_idx, char *pars)
/*---------------------------------------------------------------------------------------------------------*\
  This function will
\*---------------------------------------------------------------------------------------------------------*/
{

    return(0);
}
// EOF

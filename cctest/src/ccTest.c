/*---------------------------------------------------------------------------------------------------------*\
  File:     ccTest.c                                                                    Copyright CERN 2014

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

  Purpose:  Test program for libfg and libreg (function generation and regulation)

  Authors:  Quentin.King@cern.ch
            Stephen.Page@cern.ch

\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>

// Declare all variables in ccTest.c

#define GLOBALS

// Include cctest program header files

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRun.h"
#include "ccRef.h"
#include "ccSigs.h"

// Default commands to run on start-up

char *default_commands[] =
{
    "GLOBAL GROUP   sandbox",
    "GLOBAL PROJECT FG",
    "GLOBAL FILE    cctest",
    NULL
};

/*---------------------------------------------------------------------------------------------------------*/
int main(int argc, char **argv)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t exit_status = EXIT_SUCCESS;
    char   **cmd;
    char     line[40];

    puts("\nWelcome to cctest\n");

    // Get path to cctest project root

    sprintf(cctest.base_path, "%s/../../",dirname(argv[0]));

    // Set default group, project and filename

    for(cmd = default_commands ; *cmd != NULL ; cmd++)
    {
        if(ccTestParseLine(strcpy(line,*cmd)) == EXIT_FAILURE)
        {
            exit(EXIT_FAILURE);
        }
    }

    // If no arguments supplied, read from stdin

    if(argc == 1)
    {
        cctest.input[0].line_number++;

        exit_status = ccTestParseLine("read");
    }
    else
    {
        // else process all arguments unless an error is reported

        while(exit_status == EXIT_SUCCESS && --argc > 0)
        {
            cctest.input[0].line_number++;

            exit_status = ccTestParseLine(*(++argv));
        }
    }

    // Report exit status

    exit(exit_status);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccTestParseLine(char *line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to set the current directory using the supplied parameter
\*---------------------------------------------------------------------------------------------------------*/
{
    int32_t      idx;
    int32_t      cmd_idx;
    char        *command;
    size_t       command_len;
    struct cccmds *cmd;

    // Skip leading white space and ignore empty lines or comment (#) lines

    command = line + strspn(line, " \t");

    if(*command == '\n' || *command == '\0' || *command == '#')
    {
        return(EXIT_SUCCESS);
    }

    // Get first argument from line

    command_len = strcspn(command, CC_ARG_DELIMITER);

    line = command + command_len;

    if(*line != '\0')
    {
        *(line++) = '\0';

        line += strspn(line, CC_ARG_DELIMITER);
    }

    // If no more arguments then set line to NULL

    if(*line == '\0')
    {
        line = NULL;
    }

    // Compare first argument against list of commands

    for(cmd = cmds, cmd_idx = -1, idx = 0 ; cmd->name != NULL ; cmd++, idx++)
    {
         // If command argument matches start or all of a command

         if(strncasecmp(cmd->name, command, command_len) == 0)
         {
             // If first match, remember command index

             if(cmd_idx == -1)
             {
                 cmd_idx = idx;
             }
             else // else second match so report error
             {
                 ccTestPrintError("ambiguous command '%s'", command);
                 return(EXIT_FAILURE);
             }
         }
    }

    // If unambiguous match with a command, run the associated command function

    if(cmd_idx >= 0)
    {
        return(cmds[cmd_idx].cmd_func(cmd_idx, &line));
    }
    else
    {
         ccTestPrintError("unknown command '%s'", ccTestAbbreviatedArg(command));
         return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
char * ccTestGetArgument(char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will return the next argument in the line, delimited by white space or a comma. It returns
  NULL if there are no more arguments and *remaining_line points the start of the next argument, or is NULL
  if there are no more arguments.
\*---------------------------------------------------------------------------------------------------------*/
{
    char *arg = *remaining_line;
    char *remains;

    // if more arguments remain on the line

    if(arg != NULL)
    {
        remains = arg + strcspn(arg, CC_ARG_DELIMITER);

        // Nul terminate the new argument and skip trailing delimiter characters

        if(*remains != '\0')
        {
            *(remains++) = '\0';

            remains += strspn(remains, CC_ARG_DELIMITER);
        }

        // If no more arguments on the line then set *remaining_line to NULL

        if(*remains == '\0')
        {
            *remaining_line = NULL;
        }
        else
        {
            *remaining_line = remains;
        }
    }

    return(arg);
}
/*---------------------------------------------------------------------------------------------------------*/
void ccTestPrintError(const char * format, ...)
/*---------------------------------------------------------------------------------------------------------*\
  This function print an error message to stdout. It adds an appropriate prefix according to the source of
  the input line being processed.
\*---------------------------------------------------------------------------------------------------------*/
{
    va_list     argv;

    // If processing command line arguments

    if(cctest.input_idx == 0)
    {
        printf("Error at argument %u - ",cctest.input[0].line_number);
    }
    else
    {
        // If reading from stdin

        if(cctest.input[cctest.input_idx].line_number == 0)
        {
            printf("Error - ");
        }
        else // else reading from file
        {
            printf("Error at %s:%u - ",
                    cctest.input[cctest.input_idx].path,
                    cctest.input[cctest.input_idx].line_number);
        }
    }

    // Print error message

    va_start(argv, format);
    vprintf(format, argv);
    va_end(argv);

    // Write newline

    putchar('\n');
}
/*---------------------------------------------------------------------------------------------------------*/
char *ccTestAbbreviatedArg(char *arg)
/*---------------------------------------------------------------------------------------------------------*\
  This function will check if an argument is too long and will truncate with "..." if it is.  This makes 
  it safe to print.
\*---------------------------------------------------------------------------------------------------------*/
{
    if(strlen(arg) > CC_ABBREVIATED_ARG_LEN)
    {
        arg[CC_ABBREVIATED_ARG_LEN-4] = '.';
        arg[CC_ABBREVIATED_ARG_LEN-3] = '.';
        arg[CC_ABBREVIATED_ARG_LEN-2] = '.';
        arg[CC_ABBREVIATED_ARG_LEN-1] = '\0';
    }

    return(arg);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccTestNoMoreArgs(char **remaining_line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will check that there are no more arguments on the line. If there are, it will print an
  error message.
\*---------------------------------------------------------------------------------------------------------*/
{
    char *arg;

    arg = ccTestGetArgument(remaining_line);

    if(arg != NULL)
    {
        ccTestPrintError("unexpected argument '%s'", ccTestAbbreviatedArg(arg));
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
// EOF

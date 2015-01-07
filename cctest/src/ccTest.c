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
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>

// Declare all variables in ccTest.c

#define GLOBALS

// Include cctest program header files

#include "ccCmds.h"
#include "ccTest.h"
#include "ccInit.h"
#include "ccRun.h"
#include "ccRef.h"
#include "ccSigs.h"
#include "ccFlot.h"

// mkdir command

// On Windows the system() command in MinGW uses cmd.exe which finds the Windows mkdir in the path
// instead of the msys/bin/mkdir. To work around this, a second copy of mkdir is created in msys/bin
// which is used on Windows systems.

#ifdef __MINGW32__
#define MKDIR   "mkdir2"
#else
#define MKDIR   "mkdir"
#endif

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

    printf("\nWelcome to cctest v%.2f\n", CC_VERSION);

    // Get path to cctest project root

    ccTestGetBasePath(argv[0]);

    // Initialise parameter num_elements arrays

    ccInitPars();

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
static uint32_t ccTestParseIndex(char **line, char delimiter, uint32_t *index)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to translate a cycle selector in () or an array index in [].
\*---------------------------------------------------------------------------------------------------------*/
{
    long int value;
    char    *index_string;
    char    *remaining_string;

    // Skip leading white space before index

    index_string = *line + strspn(*line, " \t");

    // Report failure if end of line reached

    if(*index_string == '\0')
    {
        return(EXIT_FAILURE);
    }

    // If delimiters contain no value

    if(*index_string == delimiter)
    {
        if(delimiter == ')')
        {
            // Empty cycle selector "()" means "all cycles"

            *index = CC_ALL_CYCLES;
            *line = index_string + 1;

            return(EXIT_SUCCESS);
        }

        // Empty array index "[]" is not allowed

        return(EXIT_FAILURE);
    }

    // Try to translate next characters into an integer

    errno = 0;

    value = strtol(index_string, &remaining_string, 10);

    // Skip any trailing white space

    index_string = remaining_string + strspn(remaining_string, " \t");

    // Check trailing delimiter is valid and that integer was successfully converted,
    // and if it is a cycle selector, that it is not out of range

     if(*index_string != delimiter                      ||
         errno != 0                                     ||
         value  < 0                                     ||
        ((delimiter == ')' && value >  CC_MAX_CYC_SEL)) ||
        ((delimiter == ']' && value >= TABLE_LEN     )))
     {
         return(EXIT_FAILURE);
     }

     *index = (uint32_t)value;
     *line  = index_string + 1;
     return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
static uint32_t ccTestParseArg(char **line, size_t *arg_len)
/*---------------------------------------------------------------------------------------------------------*/
{
    char     delimiter;
    char    *remaining_string = *line;

    *arg_len = strcspn(remaining_string, CC_ARG_DELIMITER "([");

    remaining_string += *arg_len;

    delimiter = *remaining_string;

    if(delimiter != '\0')
    {
        // Nul terminate command string

        *(remaining_string++) = '\0';

        // If cycle selector is provided then parse the value

        if(delimiter == '(')
        {
            if(ccTestParseIndex(&remaining_string, ')', &cctest.cyc_sel) == EXIT_FAILURE)
            {
                ccTestPrintError("invalid command cycle selector");
                return(EXIT_FAILURE);
            }

            delimiter = *(remaining_string++);
        }

        // If array index is provided then parse the value

        if(delimiter == '[')
        {
            if(ccTestParseIndex(&remaining_string, ']', &cctest.array_idx) == EXIT_FAILURE)
            {
                ccTestPrintError("invalid command array index");
                return(EXIT_FAILURE);
            }
        }

        remaining_string += strspn(remaining_string, CC_ARG_DELIMITER);
    }

    // If no more arguments then set line to NULL

    if(*remaining_string == '\0')
    {
        remaining_string = NULL;
    }

    *line = remaining_string;

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccTestParseLine(char *line)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to set the current directory using the supplied parameter
\*---------------------------------------------------------------------------------------------------------*/
{
    int32_t        idx;
    int32_t        cmd_idx;
    char          *command;
    char          *remaining_string;
    size_t         command_len;
    struct cccmds *cmd;

    // Skip leading white space and ignore empty lines or comment (#) lines

    remaining_string = command = line + strspn(line, " \t");

    if(*command == '\n' || *command == '\0' || *command == '#')
    {
        return(EXIT_SUCCESS);
    }

    // Initialise cycle selectors and array indexes

    cctest.cyc_sel   = CC_NO_INDEX;
    cctest.array_idx = CC_NO_INDEX;

    // Get first argument from line (command name) and optional cycle selector and array index

    if(ccTestParseArg(&remaining_string, &command_len) == EXIT_FAILURE)
    {
        return(EXIT_FAILURE);
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
        return(cmds[cmd_idx].cmd_func(cmd_idx, &remaining_string));
    }
    else
    {
         ccTestPrintError("unknown command '%s'", ccTestAbbreviatedArg(command));
         return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccTestGetParName(uint32_t cmd_idx, char **remaining_line, struct ccpars **par_matched)
/*---------------------------------------------------------------------------------------------------------*\
  This function analyse the next argument on the line which must be empty or a valid parameter name
\*---------------------------------------------------------------------------------------------------------*/
{
    size_t              par_string_len;
    char               *par_string = *remaining_line;
    struct ccpars      *par;

    // When a parameter name is provided, the command may not have a cycle selector or array index

    if(cctest.cyc_sel != CC_NO_INDEX)
    {
        ccTestPrintError("unexpected command cycle selector");
        return(EXIT_FAILURE);
    }

    if(cctest.array_idx != CC_NO_INDEX)
    {
        ccTestPrintError("unexpected command array index");
        return(EXIT_FAILURE);
    }

    // Get next argument which is the parameter name and allow cycle selector and array index to be specified

    if(ccTestParseArg(remaining_line, &par_string_len) == EXIT_FAILURE)
    {
        return(EXIT_FAILURE);
    }

    // Analyse argument to see if it is a known parameter name for the command.

    for(par = cmds[cmd_idx].pars, *par_matched = NULL ; par->name != NULL ; par++)
    {
         // If command argument matches start or all of a command

        if(strncasecmp(par->name, par_string, par_string_len) == 0)
        {
            // If argument exactly matches a parameter name then take use it

            if(strlen(par->name) == par_string_len)
            {
                *par_matched = par;
                break;
            }

            // If first partial match, remember command index

            if(*par_matched == NULL)
            {
                *par_matched = par;
            }
            else // else second partial match so report error
            {
                ccTestPrintError("ambiguous %s parameter '%s'", cmds[cmd_idx].name, par_string);
                return(EXIT_FAILURE);
            }
        }
    }

    if(*par_matched == NULL)
    {
        ccTestPrintError("unknown parameter for %s: '%s'", cmds[cmd_idx].name, ccTestAbbreviatedArg(par_string));
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
    char *remaining_string;

    // if more arguments remain on the line

    if(arg != NULL)
    {
        remaining_string = arg + strcspn(arg, CC_ARG_DELIMITER);

        // Nul terminate the new argument and skip trailing delimiter characters

        if(*remaining_string != '\0')
        {
            *(remaining_string++) = '\0';

            remaining_string += strspn(remaining_string, CC_ARG_DELIMITER);
        }

        // If no more arguments on the line then set *remaining_line to NULL

        if(*remaining_string == '\0')
        {
            *remaining_line = NULL;
        }
        else
        {
            *remaining_line = remaining_string;
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
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccTestReadAllFiles(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function read the current working directory and the try to read each file in it.
\*---------------------------------------------------------------------------------------------------------*/
{
    struct dirent   *files;
    struct stat      dir_file_stat;
    DIR             *dp;
    char            *filename;

    // Try to open current working directory

    dp = opendir(".");

    if(dp == NULL)
    {
        printf("Fatal - failed to open current working directory : %s (%d)", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }

    // Read contents of directory - this is not re-entrant because readdir_r() is not available in MinGW

    while((files = readdir(dp)) != NULL)
    {
        filename = files->d_name;

        // Get status of each file

        if(stat(filename, &dir_file_stat) == -1)
        {
            printf("Fatal - failed to stat '%s' in current working directory : %s (%d)", filename, strerror(errno), errno);
            exit(EXIT_FAILURE);
        }

        // If regulation file

        if(S_ISREG(dir_file_stat.st_mode))
        {
            if(ccCmdsRead(0, &filename) == EXIT_FAILURE)
            {
                // Close director and report failure

                closedir(dp);
                return(EXIT_FAILURE);
            }
        }
    }

    if(errno != 0)
    {
        printf("Fatal - failed to read current working directory : %s (%d)", strerror(errno), errno);
        exit(EXIT_FAILURE);
    }

    // Close directory and report success

    closedir(dp);
    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccTestMakePath(char *path)
/*---------------------------------------------------------------------------------------------------------*\
  This function will use mkdir -p to create a path if it doesn't exist
\*---------------------------------------------------------------------------------------------------------*/
{
    struct stat      path_stat;
    char             mkdir_cmd[CC_PATH_LEN];

    // Get status of path

    if(stat(path, &path_stat) == -1)
    {
        // If path doesn't exist then try to create it

        snprintf(mkdir_cmd, CC_PATH_LEN, MKDIR " -p %s", path);

        printf("Creating path: %s\n", path);

        if(system(mkdir_cmd) != 0)
        {
            return(EXIT_FAILURE);
        }

        return(EXIT_SUCCESS);
    }

    // Report error if the path is not a directory

    if(!S_ISDIR(path_stat.st_mode))
    {
        ccTestPrintError("output path '%s' is not valid", path);
        return(EXIT_FAILURE);
    }

    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
void ccTestRecoverPath(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to recover the initial path where it is written by the CD command.
\*---------------------------------------------------------------------------------------------------------*/
{
    FILE   *f;
    char    cwd_buf[CC_PATH_LEN];

    // Try to open the file with the path

    snprintf(cctest.cwd_file_path, CC_PATH_LEN, "%s/%s", cctest.base_path, CC_CWD_FILE);

    f = fopen(cctest.cwd_file_path, "r");

    if(f == NULL)
    {
        return;
    }

    // Try to read path from start of file - ignore errors

    if(fgets(cwd_buf, CC_PATH_LEN, f) != NULL)
    {
        chdir(cwd_buf);
    }

    fclose(f);
}
/*---------------------------------------------------------------------------------------------------------*/
void ccTestGetBasePath(char *argv0)
/*---------------------------------------------------------------------------------------------------------*\
  This function will try to get the absolute path to the cctest project directory
\*---------------------------------------------------------------------------------------------------------*/
{
    // Get path to cctest executable

    snprintf(cctest.base_path, CC_PATH_LEN, "%s/../..",dirname(argv0));

    // If not Windows, path may be relative or absolute

#ifndef __MINGW32__
    if(cctest.base_path[0] != '/')
    {
        char    cwd[CC_PATH_LEN];
        char    base_path[CC_PATH_LEN];

        // Get absolute path to current working directory

        if(getcwd(cwd, sizeof(cwd)) == NULL)
        {
            printf("Fatal - getting current directory : %s (%d)\n", strerror(errno), errno);
            exit(EXIT_FAILURE);
        }

        // Concatenate with relative project path

        snprintf(base_path, CC_PATH_LEN, "%s/%s", cwd, cctest.base_path );
        strcpy(cctest.base_path, base_path);
    }
#endif
}

// EOF

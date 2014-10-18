/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/ccTest.h                                                         Copyright CERN 2014

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

  Purpose:  Header file for ccTest.c

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCTEST_H
#define CCTEST_H

#include <stdint.h>

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCTEST_EXT
#else
#define CCTEST_EXT extern
#endif

// Constants

#define CC_VERSION                  5.04
#define CC_PATH_LEN                 256
#define CC_ABBREVIATED_ARG_LEN      20
#define CC_INPUT_FILE_NEST_LIMIT    4
#define CC_ARG_DELIMITER            ", \t\n"
#define CC_CWD_FILE                 ".cctest_cwd"

// Global i/o structure

struct cctest_input
{
    uint32_t                line_number;
    char                   *path;
};

struct cctest
{
    uint32_t                input_idx;
    struct cctest_input     input        [CC_INPUT_FILE_NEST_LIMIT];
    char                    base_path    [CC_PATH_LEN];
    char                    cwd_file_path[CC_PATH_LEN];
    FILE                   *csv_file;
};

CCTEST_EXT struct cctest cctest;

// Function declarations

uint32_t ccTestParseLine        (char *line);
char    *ccTestGetArgument      (char **remaining_line);
void     ccTestPrintError       (const char * format, ...);
char    *ccTestAbbreviatedArg   (char *arg);
uint32_t ccTestNoMoreArgs       (char **remaining_line);
uint32_t ccTestReadAllFiles     (void);
uint32_t ccTestMakePath         (char *path);
void     ccTestRecoverPath      (void);
void     ccTestGetBasePath      (char *argv0);

#endif
// EOF

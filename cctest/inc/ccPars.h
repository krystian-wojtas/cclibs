/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/ccPars.h                                                         Copyright CERN 2014

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

  Purpose:  Header file for ccpars.c

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_H
#define CCPARS_H

#include <stdint.h>

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_EXT
#else
#define CCPARS_EXT extern
#endif

// Constants

#define PARS_INDENT             31
#define PARS_MAX_PRINT_LINE_LEN (CC_MAX_FILE_LINE_LEN*8)      // Allow for longest print line for table
#define PARS_MAX_REPORT_LINES   1000

// Enums

enum ccpars_type
{
    PAR_UNSIGNED,
    PAR_FLOAT,
    PAR_DOUBLE,
    PAR_STRING,
    PAR_ENUM
};

// Structures

struct ccpars
{
    char               *name;
    enum ccpars_type    type;
    uint32_t            max_num_elements;
    uint32_t            min_num_elements;
    struct ccpars_enum *ccpars_enum;
    union
    {
        double         *d;
        float          *f;
        uint32_t       *i;
        char          **s;
    } value_p;

    uint32_t            num_elements;
};

struct ccpars_enum
{
    uint32_t            value;
    char *              string;
};

struct ccpars_report
{
    uint32_t            num_lines;
    char *              line_buf[PARS_MAX_REPORT_LINES];
};

CCPARS_EXT struct ccpars_report ccpars_report;

// ENABLED/DISABLED enum

enum cc_enabled_disabled
{
    CC_DISABLED,
    CC_ENABLED
};

CCPARS_EXT struct ccpars_enum enabled_disabled[]
#ifdef GLOBALS
= {
    { CC_DISABLED,      "DISABLED"      },
    { CC_ENABLED,       "ENABLED"       },
    { 0,                NULL            },
}
#endif
;

// Function declarations

uint32_t ccParsGet                  (char *cmd_name, struct ccpars *par, char **remaining_line);
char    *ccParsEnumString           (struct ccpars_enum *par_enum, uint32_t value);
void     ccParsGenerateFlotReport   (FILE *f);
uint32_t ccParsCheckMissingPars     (uint32_t cmd_idx);
void     ccParsPrintAll             (FILE *f, char *cmd_name, struct ccpars *par);
void     ccParsPrint                (FILE *f, char *cmd_name, struct ccpars *par);
void     ccParsPrintFlotFooter      (FILE *f);
void     ccParsPrintDebug           (FILE *f);

#endif
// EOF

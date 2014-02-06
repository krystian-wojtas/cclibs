/*---------------------------------------------------------------------------------------------------------*\
  File:     ccpars.h                                                                    Copyright CERN 2011

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

  Purpose:  Header file for FG library test program parameter file parsing functions

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

#define PARS_MAX_FILE_LINE_LEN  65536
#define PARS_INDENT             30
#define PARS_MAX_PRINT_LINE_LEN (PARS_MAX_FILE_LINE_LEN*8)      // Allow for longest print line for table
#define PARS_MAX_REPORT_LINES   1000

// Parameter table structure

enum ccpars_type
{
    PAR_UNSIGNED,
    PAR_FLOAT,
    PAR_STRING,
    PAR_ENUM
};

struct ccpars
{
    char               *name;
    enum ccpars_type    type;
    uint32_t            max_values;
    uint32_t            min_values;
    struct ccpars_enum *ccpars_enum;
    union
    {
        float          *f;
        uint32_t       *i;
        char          **s;
    } value_p;

    uint32_t            default_values;
    uint32_t            num_values;
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

enum fg_enabled_disabled
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

char  * ccparsEnumString        (struct ccpars_enum *par_enum, uint32_t value);
void    ccparsGet               (int argc, char **argv);
void    ccparsGenerateReport    (void);
void    ccparsPrintReport       (FILE *f);

#endif
/*---------------------------------------------------------------------------------------------------------*\
  End of file: ccpars.h
\*---------------------------------------------------------------------------------------------------------*/


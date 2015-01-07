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
#include <libreg.h>
#include <stdbool.h>

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_EXT
#else
#define CCPARS_EXT extern
#endif

// Constants

#define PARS_INDENT                 34
#define PARS_MAX_PRINT_LINE_LEN     (CC_MAX_FILE_LINE_LEN*8)      // Allow for longest print line for table
#define PARS_MAX_REPORT_LINES       1000
#define PARS_INT_FORMAT             "% d"
#define PARS_FLOAT_FORMAT           "% .6E"
#define PARS_TIME_FORMAT            "% .6f"
#define PARS_STRING_FORMAT          " %s"

// struct ccpars flags

#define PARS_FIXED_LENGTH           0x01

// Enums

enum ccpars_type
{
    PAR_UNSIGNED,
    PAR_FLOAT,
    PAR_STRING,
    PAR_ENUM
};

// Size of type array

CCPARS_EXT uint32_t ccpars_sizeof_type[]
#ifdef GLOBALS
= {
    sizeof(uint32_t),
    sizeof(float),
    sizeof(char *),
    sizeof(uint32_t)
}
#endif
;
// Structures

struct ccpars
{
    char               *name;
    enum ccpars_type    type;
    uint32_t            max_num_elements;
    struct ccpars_enum *ccpars_enum;
    union value_p
    {
        char           *c;
        uint32_t       *u;
        float          *f;
        char          **s;
    } value_p;

    uint32_t            num_default_elements;
    uint32_t            cyc_sel_step;
    uint32_t            flags;
    uint32_t           *num_elements;           // Initialised by ccInitParNumElements()
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

CCPARS_EXT struct ccpars_enum enum_enabled_disabled[]
#ifdef GLOBALS
= {
    { REG_DISABLED,     "DISABLED"      },
    { REG_ENABLED,      "ENABLED"       },
    { 0,                NULL            },
}
#endif
;

// Function declarations

uint32_t ccParsGet                  (char *cmd_name, struct ccpars *par, char **remaining_line);
char    *ccParsEnumString           (struct ccpars_enum *par_enum, uint32_t value);
void     ccParsPrint                (FILE *f, char *cmd_name, struct ccpars *par, uint32_t cyc_sel, uint32_t array_idx);
void     ccParsPrintAll             (FILE *f, char *cmd_name, struct ccpars *par, uint32_t cyc_sel, uint32_t array_idx);

#endif
// EOF

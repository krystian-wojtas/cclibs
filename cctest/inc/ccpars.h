/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/ccpars.h                                                         Copyright CERN 2014

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

// Enums

enum ccpars_type
{
    PAR_UNSIGNED,
    PAR_FLOAT,
    PAR_STRING,
    PAR_ENUM
};

// Structures

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

struct ccpars_group
{
    char                *name;
    struct ccpars       *pars;
    uint32_t            n_pars_read;
    uint32_t            n_pars_missing;
    uint32_t            enabled;
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

// Parameter groups - the enum ccpars_groups_enum order must match the ccpars_groups array below

enum ccpars_groups_enum
{
    GROUP_GLOBAL,
    GROUP_LIMITS,
    GROUP_LOAD,
    GROUP_MEAS,
    GROUP_REG_B,
    GROUP_REG_I,
    GROUP_VS,
    GROUP_START,
    GROUP_PLEP,
    GROUP_RAMP,
    GROUP_PPPL,
    GROUP_TABLE,
    GROUP_TRIM,
    GROUP_TEST,
    N_GROUPS
};

// Include libfg and libreg header files - required by ccpars_groups[]

#include "libreg.h"

// Include cctest function data header files

#include "func/start.h"
#include "func/plep.h"
#include "func/ramp.h"
#include "func/pppl.h"
#include "func/table.h"
#include "func/trim.h"
#include "func/test.h"

// Include cctest parameter header files - required by ccpars_groups[]

#include "pars/global.h"
#include "pars/limits.h"
#include "pars/load.h"
#include "pars/meas.h"
#include "pars/reg.h"
#include "pars/vs.h"

// Define array of parameter groups

struct ccpars_group ccpars_groups[N_GROUPS+1] // Must be in enum ccpars_groups_enum order!
#ifdef GLOBALS
= {
    { "GLOBAL", global_pars_list },
    { "LIMITS", limits_pars_list },
    { "LOAD",   load_pars_list   },
    { "MEAS",   meas_pars_list   },
    { "REG_B",  reg_b_pars_list  },
    { "REG_I",  reg_i_pars_list  },
    { "VS",     vs_pars_list     },
    { "START",  start_pars_list  },
    { "PLEP",   plep_pars_list   },
    { "RAMP",   ramp_pars_list   },
    { "PPPL",   pppl_pars_list   },
    { "TABLE",  table_pars_list  },
    { "TRIM",   trim_pars_list   },
    { "TEST",   test_pars_list   },
    { NULL }
}
#endif
;

// Function declarations

char  * ccparsEnumString        (struct ccpars_enum *par_enum, uint32_t value);
void    ccparsGet               (int argc, char **argv);
void    ccparsGenerateReport    (void);
void    ccparsPrintReport       (FILE *f);

#endif
// EOF

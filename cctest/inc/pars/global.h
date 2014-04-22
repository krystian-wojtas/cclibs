/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/pars/global.h                                                    Copyright CERN 2014

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

  Purpose:  Structure for the global parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_GLOBAL_H
#define CCPARS_GLOBAL_H

#include "ccCmds.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_GLOBAL_EXT
#else
#define CCPARS_GLOBAL_EXT extern
#endif

// Constants

#define MAX_FUNCS               10              // Max number of functions than can be chained together
#define ZERO_MEAS_FACTOR        0.01            //  1% of positive limit
#define LOW_MEAS_FACTOR         0.1             // 10% of positive limit

// Function type enum - must match order of struct fgfunc funcs[] in ccRef.h

enum fg_types
{
    FG_START,
    FG_PLEP,
    FG_RAMP,
    FG_PPPL,
    FG_TABLE,
    FG_STEPS,
    FG_SQUARE,
    FG_SINE,
    FG_COSINE,
    FG_LTRIM,
    FG_CTRIM,
};

CCPARS_GLOBAL_EXT struct ccpars_enum function_type[]
#ifdef GLOBALS
= {
    { FG_START,       "START"  },
    { FG_PLEP,        "PLEP"   },
    { FG_RAMP,        "RAMP"   },
    { FG_PPPL,        "PPPL"   },
    { FG_TABLE,       "TABLE"  },
    { FG_STEPS,       "STEPS"  },
    { FG_SQUARE,      "SQUARE" },
    { FG_SINE,        "SINE"   },
    { FG_COSINE,      "COSINE" },
    { FG_LTRIM,       "LTRIM"  },
    { FG_CTRIM,       "CTRIM"  },
    { 0,               NULL    },
}
#endif
;

// Regulation mode enum constants from libreg

CCPARS_GLOBAL_EXT struct ccpars_enum reg_mode[]
#ifdef GLOBALS
= {
    { REG_VOLTAGE,    "VOLTAGE" },
    { REG_CURRENT,    "CURRENT" },
    { REG_FIELD,      "FIELD"   },
    { 0,               NULL     },
}
#endif
;

// Output format enum

enum cc_csv_format
{
    CC_NONE,
    CC_STANDARD,
    CC_FGCSPY,
    CC_LVDV,
};

CCPARS_GLOBAL_EXT struct ccpars_enum csv_format[]
#ifdef GLOBALS
= {
    { CC_NONE,         "NONE"     },
    { CC_STANDARD,     "STANDARD" },
    { CC_FGCSPY,       "FGCSPY"   },
    { CC_LVDV,         "LVDV"     },
    { 0,               NULL       },
}
#endif
;

// Global parameters structure

struct ccpars_global
{
    float               pre_func_delay;     // Pre-function time delay (time added between functions).
    float               run_delay;          // Delay given to libfg for each function
    float               stop_delay;         // Time after end of last ref function
    float               iter_period;        // Global iteration period
    float               abort_time;         // Time to abort the ref function (limits are required)
    uint32_t            reverse_time;       // Reverse time flag (tests ref function with decreasing time)
    uint32_t            reg_mode[MAX_FUNCS];// Regulation modes (VOLTAGE, CURRENT or FIELD)
    uint32_t            function[MAX_FUNCS];// Ref function types
    uint32_t            fg_limits;          // Enable limits for function generator initialisation
    uint32_t            sim_load;           // Enable load simulation
    uint32_t            csv_format;         // CSV output data format
    uint32_t            flot_control;       // FLOT webplot output control (ENABLED or DISABLED)
    char *              group;              // Test group name (e.g. sandbox or tests)
    char *              project;            // Project name (e.g. SPS_MPS)
    char *              file;               // Results filename root (exclude .csv or .html)
};

CCPARS_GLOBAL_EXT struct ccpars_global ccpars_global
#ifdef GLOBALS
= {//   Default value           Parameter
        0.1,                 // GLOBAL PRE_FUNC_DELAY
        1.0,                 // GLOBAL RUN_DELAY
        1.0,                 // GLOBAL STOP_DELAY
        1.0E-3,              // GLOBAL ITER_PERIOD
        0.0,                 // GLOBAL ABORT_TIME
        CC_DISABLED,         // GLOBAL REVERSE_TIME
        { REG_VOLTAGE },     // GLOBAL REG_MODE
        { FG_SINE },         // GLOBAL FUNCTION
        CC_DISABLED,         // GLOBAL FG_LIMITS
        CC_DISABLED,         // GLOBAL SIM_LOAD
        CC_NONE,             // GLOBAL CSV_FORMAT
        CC_ENABLED,          // GLOBAL FLOT_OUTPUT
        "sandbox",           // GLOBAL GROUP
        "FG",                // GLOBAL PROJECT
        "cctest"             // GLOBAL FILE
}
#endif
;

// Global parameters description structure

enum global_pars_index_enum
{
    GLOBAL_PRE_FUNC_DELAY    ,
    GLOBAL_RUN_DELAY         ,
    GLOBAL_STOP_DELAY        ,
    GLOBAL_ITER_PERIOD       ,
    GLOBAL_ABORT_TIME        ,
    GLOBAL_REVERSE_TIME      ,
    GLOBAL_REG_MODE          ,
    GLOBAL_FUNCTION          ,
    GLOBAL_FG_LIMITS         ,
    GLOBAL_SIM_LOAD          ,
    GLOBAL_CSV_FORMAT        ,
    GLOBAL_FLOT_OUTPUT       ,
    GLOBAL_GROUP             ,
    GLOBAL_PROJECT           ,
    GLOBAL_FILE
};

CCPARS_GLOBAL_EXT struct ccpars global_pars[]
#ifdef GLOBALS
= {// "Signal name"        type,      max_n_els, min_n_els,*enum,             *value,                        num_defaults
    { "PRE_FUNC_DELAY",    PAR_FLOAT,        1, 1, NULL,             { .f = &ccpars_global.pre_func_delay     }, 1 },
    { "RUN_DELAY",         PAR_FLOAT,        1, 1, NULL,             { .f = &ccpars_global.run_delay          }, 1 },
    { "STOP_DELAY",        PAR_FLOAT,        1, 1, NULL,             { .f = &ccpars_global.stop_delay         }, 1 },
    { "ITER_PERIOD",       PAR_FLOAT,        1, 1, NULL,             { .f = &ccpars_global.iter_period        }, 1 },
    { "ABORT_TIME",        PAR_FLOAT,        1, 1, NULL,             { .f = &ccpars_global.abort_time         }, 1 },
    { "REVERSE_TIME",      PAR_ENUM,         1, 1, enabled_disabled, { .i = &ccpars_global.reverse_time       }, 1 },
    { "REG_MODE",          PAR_ENUM, MAX_FUNCS, 1, reg_mode,         { .i =  ccpars_global.reg_mode           }, 1 },
    { "FUNCTION",          PAR_ENUM, MAX_FUNCS, 1, function_type,    { .i =  ccpars_global.function           }, 1 },
    { "FG_LIMITS",         PAR_ENUM,         1, 1, enabled_disabled, { .i = &ccpars_global.fg_limits          }, 1 },
    { "SIM_LOAD",          PAR_ENUM,         1, 1, enabled_disabled, { .i = &ccpars_global.sim_load           }, 1 },
    { "CSV_FORMAT",        PAR_ENUM,         1, 1, csv_format,       { .i = &ccpars_global.csv_format         }, 1 },
    { "FLOT_OUTPUT",       PAR_ENUM,         1, 1, enabled_disabled, { .i = &ccpars_global.flot_control       }, 1 },
    { "GROUP",             PAR_STRING,       1, 1, NULL,             { .s = &ccpars_global.group              }, 1 },
    { "PROJECT",           PAR_STRING,       1, 1, NULL,             { .s = &ccpars_global.project            }, 1 },
    { "FILE",              PAR_STRING,       1, 1, NULL,             { .s = &ccpars_global.file               }, 1 },
    { NULL }
}
#endif
;

#endif
// EOF

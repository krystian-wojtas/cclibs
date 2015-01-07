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

#include "ccPars.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_GLOBAL_EXT
#else
#define CCPARS_GLOBAL_EXT extern
#endif

// Constants

#define MAX_CYCLES  16

// Regulation error rate control enum comes from libreg

CCPARS_EXT struct ccpars_enum enum_reg_err_rate[]
#ifdef GLOBALS
= {
    { REG_ERR_RATE_REGULATION,   "REGULATION"     },
    { REG_ERR_RATE_MEASUREMENT,  "MEASUREMENT"    },
    { 0,                          NULL            },
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

CCPARS_GLOBAL_EXT struct ccpars_enum enum_csv_format[]
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
    float                       run_delay;                  // Delay given to libfg for each function
    float                       stop_delay;                 // Time after end of last ref function
    uint32_t                    iter_period_us;             // Global iteration period (us)
    float                       abort_time;                 // Time to abort the ref function (limits are required)
    uint32_t                    flot_points_max;            // Maximum number of allowed Flot points
    enum reg_enabled_disabled   reverse_time;               // Reverse time flag (tests ref function with decreasing time)
    uint32_t                    cycle_selector[MAX_CYCLES]; // Cycle selectors
    uint32_t                    test_cyc_sel;               // Cycle selector on which to use test RST parameters
    uint32_t                    test_ref_cyc_sel;           // Cycle selector for reference function when playing test_cyc_sel
    float                       dyn_eco_time[2];            // Start/end time since start of function for dynamic economy
    enum reg_err_rate           reg_err_rate;               // Regulation error rate control
    enum reg_enabled_disabled   fg_limits;                  // Enable limits for function generator initialisation
    enum reg_enabled_disabled   sim_load;                   // Enable load simulation
    enum reg_enabled_disabled   stop_on_error;              // Enable stop on error - this will stop reading the file
    enum cc_csv_format          csv_format;                 // CSV output data format
    enum reg_enabled_disabled   flot_output;                // FLOT webplot output control (ENABLED or DISABLED)
    enum reg_enabled_disabled   debug_output;               // Debug output control (ENABLED or DISABLED)
    char *                      group;                      // Test group name (e.g. sandbox or tests)
    char *                      project;                    // Project name (e.g. SPS_MPS)
    char *                      file;                       // Results filename root (exclude .csv or .html)
};

CCPARS_GLOBAL_EXT struct ccpars_global ccpars_global
#ifdef GLOBALS
= {//  Default value                 Parameter
       1.0                    ,   // GLOBAL RUN_DELAY
       1.0                    ,   // GLOBAL STOP_DELAY
       1000                   ,   // GLOBAL ITER_PERIOD_US
       0.0                    ,   // GLOBAL ABORT_TIME
       100000                 ,   // GLOBAL FLOT_POINTS_MAX
       REG_DISABLED           ,   // GLOBAL REVERSE_TIME
       { 0 }                  ,   // GLOBAL CYCLE_SELECTOR
       0                      ,   // GLOBAL TEST_CYC_SEL
       0                      ,   // GLOBAL TEST_REF_CYC_SEL
       { 0.0, 0.0 }           ,   // GLOBAL DYN_ECO_TIME
       REG_ERR_RATE_REGULATION,   // GLOBAL REG_ERR_RATE
       REG_DISABLED           ,   // GLOBAL FG_LIMITS
       REG_DISABLED           ,   // GLOBAL SIM_LOAD
       REG_ENABLED            ,   // GLOBAL STOP_ON_ERROR
       CC_NONE                ,   // GLOBAL CSV_FORMAT
       REG_ENABLED            ,   // GLOBAL FLOT_OUTPUT
       REG_ENABLED            ,   // GLOBAL DEBUG_OUTPUT
}
#endif
;

// Global parameters description structure

enum global_pars_index_enum
{
    GLOBAL_RUN_DELAY         ,
    GLOBAL_STOP_DELAY        ,
    GLOBAL_ITER_PERIOD_US    ,
    GLOBAL_ABORT_TIME        ,
    GLOBAL_FLOT_POINTS_MAX   ,
    GLOBAL_REVERSE_TIME      ,
    GLOBAL_CYCLE_SELECTOR    ,
    GLOBAL_TEST_CYC_SEL      ,
    GLOBAL_TEST_REF_CYC_SEL  ,
    GLOBAL_DYN_ECO_TIME      ,
    GLOBAL_REG_ERR_RATE      ,
    GLOBAL_FG_LIMITS         ,
    GLOBAL_SIM_LOAD          ,
    GLOBAL_STOP_ON_ERROR     ,
    GLOBAL_CSV_FORMAT        ,
    GLOBAL_FLOT_OUTPUT       ,
    GLOBAL_DEBUG_OUTPUT      ,
    GLOBAL_GROUP             ,
    GLOBAL_PROJECT           ,
    GLOBAL_FILE
};

CCPARS_GLOBAL_EXT struct ccpars global_pars[]
#ifdef GLOBALS
= {// "Signal name"      type,         max_n_els, *enum,                         *value,            num_defaults,cyc_sel_step,flags
    { "RUN_DELAY",       PAR_FLOAT,    1,          NULL,                  { .f = &ccpars_global.run_delay        }, 1, 0, 0                 },
    { "STOP_DELAY",      PAR_FLOAT,    1,          NULL,                  { .f = &ccpars_global.stop_delay       }, 1, 0, 0                 },
    { "ITER_PERIOD_US",  PAR_UNSIGNED, 1,          NULL,                  { .u = &ccpars_global.iter_period_us   }, 1, 0, 0                 },
    { "ABORT_TIME",      PAR_FLOAT,    1,          NULL,                  { .f = &ccpars_global.abort_time       }, 1, 0, 0                 },
    { "FLOT_POINTS_MAX", PAR_UNSIGNED, 1,          NULL,                  { .u = &ccpars_global.flot_points_max  }, 1, 0, 0                 },
    { "REVERSE_TIME",    PAR_ENUM,     1,          enum_enabled_disabled, { .u = &ccpars_global.reverse_time     }, 1, 0, 0                 },
    { "CYCLE_SELECTOR",  PAR_UNSIGNED, MAX_CYCLES, NULL,                  { .u =  ccpars_global.cycle_selector   }, 1, 0, 0                 },
    { "TEST_CYC_SEL",    PAR_UNSIGNED, 1,          NULL,                  { .u = &ccpars_global.test_cyc_sel     }, 1, 0, 0                 },
    { "TEST_REF_CYC_SEL",PAR_UNSIGNED, 1,          NULL,                  { .u = &ccpars_global.test_ref_cyc_sel }, 1, 0, 0                 },
    { "DYN_ECO_TIME",    PAR_FLOAT,    2,          NULL,                  { .f =  ccpars_global.dyn_eco_time     }, 2, 0, PARS_FIXED_LENGTH },
    { "REG_ERR_RATE",    PAR_ENUM,     1,          enum_reg_err_rate,     { .u = &ccpars_global.reg_err_rate     }, 1, 0, 0                 },
    { "FG_LIMITS",       PAR_ENUM,     1,          enum_enabled_disabled, { .u = &ccpars_global.fg_limits        }, 1, 0, 0                 },
    { "SIM_LOAD",        PAR_ENUM,     1,          enum_enabled_disabled, { .u = &ccpars_global.sim_load         }, 1, 0, 0                 },
    { "STOP_ON_ERROR",   PAR_ENUM,     1,          enum_enabled_disabled, { .u = &ccpars_global.stop_on_error    }, 1, 0, 0                 },
    { "CSV_FORMAT",      PAR_ENUM,     1,          enum_csv_format,       { .u = &ccpars_global.csv_format       }, 1, 0, 0                 },
    { "FLOT_OUTPUT",     PAR_ENUM,     1,          enum_enabled_disabled, { .u = &ccpars_global.flot_output      }, 1, 0, 0                 },
    { "DEBUG_OUTPUT",    PAR_ENUM,     1,          enum_enabled_disabled, { .u = &ccpars_global.debug_output     }, 1, 0, 0                 },
    { "GROUP",           PAR_STRING,   1,          NULL,                  { .s = &ccpars_global.group            }, 1, 0, 0                 },
    { "PROJECT",         PAR_STRING,   1,          NULL,                  { .s = &ccpars_global.project          }, 1, 0, 0                 },
    { "FILE",            PAR_STRING,   1,          NULL,                  { .s = &ccpars_global.file             }, 1, 0, 0                 },
    { NULL }
}
#endif
;

#endif
// EOF

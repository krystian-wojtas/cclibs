/*---------------------------------------------------------------------------------------------------------*\
  File:     pars/global.h                                                               Copyright CERN 2011

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

  Purpose:  Structure for the global parameter file (-g global_file)

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_GLOBAL_H
#define CCPARS_GLOBAL_H

#include "ccpars.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_GLOBAL_EXT
#else
#define CCPARS_GLOBAL_EXT extern
#endif

// Constants

#define ZERO_MEAS_FACTOR        0.01            //  1% of positive limit
#define LOW_MEAS_FACTOR         0.1             // 10% of positive limit

// Function type enum

enum fg_types
{
    FG_NONE,
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
    FG_CTRIM
};

CCPARS_GLOBAL_EXT struct ccpars_enum function_type[]
#ifdef GLOBALS
= {
    { FG_NONE,        "NONE"   },
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

// Function units is based on the reg_mode enum constants from libreg

CCPARS_GLOBAL_EXT struct ccpars_enum function_units[]
#ifdef GLOBALS
= {
    { REG_VOLTAGE,    "VOLTS"  },
    { REG_CURRENT,    "AMPS"   },
    { REG_FIELD,      "GAUSS"  },
    { 0,               NULL    },
}
#endif
;

// Output format

enum cc_output_format
{
    CC_STANDARD = 1,
    CC_FGCSPY,
    CC_LVDV,
    CC_FLOT,
};

CCPARS_GLOBAL_EXT struct ccpars_enum output_format[]
#ifdef GLOBALS
= {
    { CC_STANDARD,     "STANDARD" },
    { CC_FGCSPY,       "FGCSPY"   },
    { CC_LVDV,         "LVDV"     },
    { CC_FLOT,         "FLOT"     },
    { 0,               NULL       },
}
#endif
;

// Global parameters structure

struct ccpars_global
{
    // Global file parameters

    float               run_delay;        // Time before start of function
    float               stop_delay;       // Time after end of function
    float               iter_period;      // Global iteration period
    float               abort_time;       // Time to abort the function (limits are required)
    uint32_t            reverse_time;     // Reverse time flag (tests ref functions with descreasing time)
    uint32_t            units;            // Function generator units (G, A or V)
    uint32_t            function;         // Function type
    uint32_t            fg_limits;        // Enable limits for function generator initialisation
    uint32_t            sim_load;         // Enable load simulation (includes regulation if units are G or A)
    uint32_t            output_format;    // Output data format
    char *              flot_path;        // FLOT path to javascript libraries

    // Global related variables

    uint32_t            status;           // Global parameter group status
    uint32_t            func_data_status; // Function data status
    uint32_t            verbose_flag;     // -v option : Verbose enabled
    uint32_t            output_format_opt;// -o option : Output format (overrides global parameter file)
    uint32_t            num_iterations;   // Number of iterations for the simulation
};

CCPARS_GLOBAL_EXT struct ccpars_global ccpars_global
#ifdef GLOBALS
= {
    1.0,1.0,0.0,0.0,                    // No default for FG_PERIOD
    CC_DISABLED, REG_VOLTAGE, 0,        // No default for FUNCTION
    CC_DISABLED, CC_DISABLED,
    CC_FLOT,                            // Default output format
    "../.."                             // Default FLOT path
}
#endif
;

// Global parameters description structure

CCPARS_GLOBAL_EXT struct ccpars global_pars_list[]
#ifdef GLOBALS
= {// "Signal name"      TYPE,    max_vals, min_vals,*enum,        *value,                        num_defaults
    { "RUN_DELAY",       PAR_FLOAT,      1, 0, NULL,             { .f = &ccpars_global.run_delay       }, 1 },
    { "STOP_DELAY",      PAR_FLOAT,      1, 0, NULL,             { .f = &ccpars_global.stop_delay      }, 1 },
    { "ITER_PERIOD",     PAR_FLOAT,      1, 1, NULL,             { .f = &ccpars_global.iter_period     }, 0 },
    { "ABORT_TIME",      PAR_FLOAT,      1, 0, NULL,             { .f = &ccpars_global.abort_time      }, 1 },
    { "REVERSE_TIME",    PAR_ENUM,       1, 0, enabled_disabled, { .i = &ccpars_global.reverse_time    }, 1 },
    { "UNITS",           PAR_ENUM,       1, 0, function_units,   { .i = &ccpars_global.units           }, 1 },
    { "FUNCTION",        PAR_ENUM,       1, 0, function_type,    { .i = &ccpars_global.function        }, 1 },
    { "FG_LIMITS",       PAR_ENUM,       1, 0, enabled_disabled, { .i = &ccpars_global.fg_limits       }, 1 },
    { "SIM_LOAD",        PAR_ENUM,       1, 0, enabled_disabled, { .i = &ccpars_global.sim_load        }, 1 },
    { "OUTPUT_FORMAT",   PAR_ENUM,       1, 0, output_format,    { .i = &ccpars_global.output_format   }, 1 },
    { "FLOT_PATH",       PAR_STRING,     1, 0, NULL,             { .s = &ccpars_global.flot_path       }, 1 },
    { NULL }
}
#endif
;

#endif
/*---------------------------------------------------------------------------------------------------------*\
  End of file: pars/global.h
\*---------------------------------------------------------------------------------------------------------*/


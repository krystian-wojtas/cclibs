/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/pars/ref.h                                                       Copyright CERN 2014

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

  Purpose:  Structure for the reference function parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_REF_H
#define CCPARS_REF_H

#include "ccTest.h"
#include "ccPars.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_REF_EXT
#else
#define CCPARS_REF_EXT extern
#endif

// Pre-function policies enum

enum prefunc_policy
{
    PREFUNC_RAMP,
    PREFUNC_MIN,
    PREFUNC_MINMAX,
};

CCPARS_REF_EXT struct ccpars_enum enum_prefunc_policy[]
#ifdef GLOBALS
= {
    { PREFUNC_RAMP,      "RAMP"         },
    { PREFUNC_MIN,       "MIN"          },
    { PREFUNC_MINMAX,    "MINMAX"       },
    { 0,                  NULL          },
}
#endif
;

// Function type enum - must match order of struct fgfunc funcs[] in ccRef.h

enum fg_types
{
    FG_NONE,
    FG_DIRECT,
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
    FG_PULSE,
};

CCPARS_GLOBAL_EXT struct ccpars_enum enum_function_type[]
#ifdef GLOBALS
= {
    { FG_NONE,        "NONE"   },
    { FG_DIRECT,      "DIRECT" },
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
    { FG_PULSE,       "PULSE"  },
    { 0,               NULL    },
}
#endif
;

CCPARS_GLOBAL_EXT struct ccpars_enum enum_reg_mode[]
#ifdef GLOBALS
= {
    { REG_NONE,       "NONE"    },
    { REG_VOLTAGE,    "VOLTAGE" },
    { REG_CURRENT,    "CURRENT" },
    { REG_FIELD,      "FIELD"   },
    { 0,               NULL     },
}
#endif
;

CCPARS_GLOBAL_EXT struct ccpars_enum enum_fg_error[]
#ifdef GLOBALS
= {
    { FG_OK,                         "OK"                         },
    { FG_BAD_ARRAY_LEN,              "BAD_ARRAY_LEN"              },
    { FG_BAD_PARAMETER,              "BAD_PARAMETER"              },
    { FG_INVALID_TIME,               "INVALID_TIME"               },
    { FG_OUT_OF_LIMITS,              "OUT_OF_LIMITS"              },
    { FG_OUT_OF_RATE_LIMITS,         "OUT_OF_RATE_LIMITS"         },
    { FG_OUT_OF_ACCELERATION_LIMITS, "OUT_OF_ACCELERATION_LIMITS" },
    { 0,                              NULL                        },
}
#endif
;

// Ref parameters structure

struct ccpars_ref
{
    enum reg_mode               reg_mode            [CC_NUM_CYC_SELS][1];     // Regulation mode (VOLTAGE, CURRENT or FIELD)
    enum fg_types               function            [CC_NUM_CYC_SELS][1];     // Ref function type
    enum prefunc_policy         prefunc_policy      [CC_NUM_CYC_SELS][1];     // Pre-function policy
    float                       prefunc_min_ref     [CC_NUM_CYC_SELS][1];     // Minimum reference for pre-function
};

CCPARS_REF_EXT struct ccpars_ref ccpars_ref
#ifdef GLOBALS
= {//   Default value                  Parameter
    { { REG_VOLTAGE  } },           // REF REG_MODE(0)
    { { FG_SINE      } },           // REF FUNCTION(0)
    { { PREFUNC_RAMP } },           // REF PREFUNC_POLICY(0)
    { { 0.0          } },           // REF PREFUNC_MIN_REF(0)
}
#endif
;


// Ref parameters enum to allow access to named fields

enum ref_pars_index_enum
{
    REF_REG_MODE       ,
    REF_FUNCTION       ,
    REF_PREFUNC_POLICY ,
    REF_PREFUNC_MIN_REF,
};

CCPARS_GLOBAL_EXT struct ccpars ref_pars[]
#ifdef GLOBALS
= {// "Signal name"      type,  max_n_els, *enum,                       *value,                        num_defaults, flags
    { "REG_MODE",        PAR_ENUM,  1,      enum_reg_mode,       { .u = &ccpars_ref.reg_mode       [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "FUNCTION",        PAR_ENUM,  1,      enum_function_type,  { .u = &ccpars_ref.function       [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "PREFUNC_POLICY",  PAR_ENUM,  1,      enum_prefunc_policy, { .u = &ccpars_ref.prefunc_policy [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "PREFUNC_MIN_REF", PAR_FLOAT, 1,      NULL,                { .f = &ccpars_ref.prefunc_min_ref[0][0] }, 1, PARS_CYCLE_SELECTOR },
    { NULL }
}
#endif
;

#endif
// EOF


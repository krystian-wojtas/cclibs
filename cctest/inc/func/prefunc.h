/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/func/prefunc.h                                                   Copyright CERN 2014

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

  Purpose:  Structure for PREFUNC function parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_PREFUNC_H
#define CCPARS_PREFUNC_H

#include "libfg/ramp.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_PREFUNC_EXT
#else
#define CCPARS_PREFUNC_EXT extern
#endif

// Constants

#define MAX_PREFUNCS        3

// Prefunction policies enum

enum prefunc_policies
{
    PREFUNC_RAMP,
    PREFUNC_STOPSTART,
    PREFUNC_MIN,
    PREFUNC_MINMAX,
};

CCPARS_PREFUNC_EXT struct ccpars_enum prefunc_policies[]
#ifdef GLOBALS
= {
    { PREFUNC_RAMP,      "RAMP"         },
    { PREFUNC_STOPSTART, "STOPSTART"    },
    { PREFUNC_MIN,       "MIN"          },
    { PREFUNC_MINMAX,    "MINMAX"       },
    { 0,                 NULL           },
}
#endif
;

// PREFUNC data structure

struct ccpars_prefunc
{
    enum prefunc_policies       policy;                 // Prefunction policy
    float                       plateau_duration;       // Duration of plateaus between RAMPs

    // cctest PREFUNC parameters

    struct fg_ramp_config       config;                 // Libfg config struct for PREFUNC

    // Libfg PREFUNC variables

    struct fg_ramp_pars         pars;                   // Libfg parameters for PREFUNC
};

CCPARS_PREFUNC_EXT struct ccpars_prefunc ccpars_prefunc
#ifdef GLOBALS
= {//   Default value            Parameter
        PREFUNC_RAMP,         // PREFUNC POLICY
        0.1,                  // PREFUNC PLATEAU_DURATION
        {
            1.0,              // Final rate parmeter in config - there is no equivalent CCTEST parameter
            10.0,             // PREFUNC ACCELERATION
            0.0,              // PREFUNC LINEAR_RATE
            10.0,             // PREFUNC DECELERTION
        }
}
#endif
;

// PREFUNC data description structure

CCPARS_PREFUNC_EXT struct ccpars   prefunc_pars[]
#ifdef GLOBALS
= {// "Signal name"       type,     max_n_els,*enum,        *value,                       num_defaults
    { "POLICY",           PAR_ENUM,     1,     prefunc_policies, { .i = &ccpars_prefunc.policy              }, 1 },
    { "PLATEAU_DURATION", PAR_FLOAT,    1,     NULL,             { .f = &ccpars_prefunc.plateau_duration    }, 1 },
    { "ACCELERATION",     PAR_FLOAT,    1,     NULL,             { .f = &ccpars_prefunc.config.acceleration }, 1 },
    { "LINEAR_RATE",      PAR_FLOAT,    1,     NULL,             { .f = &ccpars_prefunc.config.linear_rate  }, 1 },
    { "DECELERATION",     PAR_FLOAT,    1,     NULL,             { .f = &ccpars_prefunc.config.deceleration }, 1 },
    { NULL }
}
#endif
;

#endif
// EOF


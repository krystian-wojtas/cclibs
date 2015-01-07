/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/pars/default.h                                                   Copyright CERN 2014

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

  Purpose:  Structure for the default parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_DEFAULT_H
#define CCPARS_DEFAULT_H

#include "ccPars.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_DEFAULT_EXT
#else
#define CCPARS_DEFAULT_EXT extern
#endif

// Default parameters structure

struct ccpars_default_pars
{
    float                       acceleration;         // Default acceleration
    float                       deceleration;         // Default deceleration
    float                       linear_rate ;         // Default linear_rate
};

struct ccpars_default
{
    struct ccpars_default_pars  pars[REG_NONE];       // Default parameters for REG_VOLTAGE, REG_CURRENT, REG_FIELD

    float                       plateau_duration;     // Before function (minimum) plateau durations
};

CCPARS_DEFAULT_EXT struct ccpars_default ccpars_default
#ifdef GLOBALS
= {// Default value       Parameter
    {
        {
            1.0,           // DEFAULT V_ACCELERATION
            1.0,           // DEFAULT V_DECELERATION
            0.0,           // DEFAULT V_LINEAR_RATE
        },
        {
            1.0,           // DEFAULT I_ACCELERATION
            1.0,           // DEFAULT I_DECELERATION
            0.0,           // DEFAULT I_LINEAR_RATE
        },
        {
            1.0,           // DEFAULT B_ACCELERATION
            1.0,           // DEFAULT B_DECELERATION
            0.0,           // DEFAULT B_LINEAR_RATE
        }
    },
    0.1                // DEFAULT PLATEAU_DURATION
}
#endif
;

// Default parameters description structure

CCPARS_GLOBAL_EXT struct ccpars default_pars[]
#ifdef GLOBALS
= {// "Signal name"       type,  max_n_els, *enum,        *value,                             num_defaults,cyc_sel_step,flags
    { "V_ACCELERATION",   PAR_FLOAT, 1,      NULL, { .f = &ccpars_default.pars[REG_VOLTAGE].acceleration   }, 1, 0, 0 },
    { "V_DECELERATION",   PAR_FLOAT, 1,      NULL, { .f = &ccpars_default.pars[REG_VOLTAGE].deceleration   }, 1, 0, 0 },
    { "V_LINEAR_RATE",    PAR_FLOAT, 1,      NULL, { .f = &ccpars_default.pars[REG_VOLTAGE].linear_rate    }, 1, 0, 0 },
    { "I_ACCELERATION",   PAR_FLOAT, 1,      NULL, { .f = &ccpars_default.pars[REG_CURRENT].acceleration   }, 1, 0, 0 },
    { "I_DECELERATION",   PAR_FLOAT, 1,      NULL, { .f = &ccpars_default.pars[REG_CURRENT].deceleration   }, 1, 0, 0 },
    { "I_LINEAR_RATE",    PAR_FLOAT, 1,      NULL, { .f = &ccpars_default.pars[REG_CURRENT].linear_rate    }, 1, 0, 0 },
    { "B_ACCELERATION",   PAR_FLOAT, 1,      NULL, { .f = &ccpars_default.pars[REG_FIELD].acceleration     }, 1, 0, 0 },
    { "B_DECELERATION",   PAR_FLOAT, 1,      NULL, { .f = &ccpars_default.pars[REG_FIELD].deceleration     }, 1, 0, 0 },
    { "B_LINEAR_RATE",    PAR_FLOAT, 1,      NULL, { .f = &ccpars_default.pars[REG_FIELD].linear_rate      }, 1, 0, 0 },
    { "PLATEAU_DURATION", PAR_FLOAT, 1,      NULL, { .f = &ccpars_default.plateau_duration                 }, 1, 0, 0 },
    { NULL }
}
#endif
;

#endif
// EOF


/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/func/ramp.h                                                      Copyright CERN 2014

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

  Purpose:  Structure for RAMP function parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_RAMP_H
#define CCPARS_RAMP_H

#include "libfg/ramp.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_RAMP_EXT
#else
#define CCPARS_RAMP_EXT extern
#endif

// RAMP data structure

struct ccpars_ramp
{
    // RAMP file data

    float                       initial_ref;
    struct fg_ramp_config       config;                     // Libfg config struct for RAMP

    // Libfg RAMP variables

    struct fg_ramp_pars         ramp_pars;                  // Libfg parameters for RAMP
};

CCPARS_RAMP_EXT struct ccpars_ramp ccpars_ramp
#ifdef GLOBALS
= {//   Default value           Parameter
        0.0,                 // RAMP INITIAL_REF
        {
            1.0,             // RAMP FINAL_REF
            4.0,             // RAMP ACCELERATION
            1.0,             // RAMP LINEAR_RATE
            6.0,             // RAMP DECELERTION
        }
}
#endif
;

// RAMP data description structure

CCPARS_RAMP_EXT struct ccpars   ramp_pars[]
#ifdef GLOBALS
= {// "Signal name"   type, max_n_els,min_n_els,*enum,        *value,                       num_defaults
    { "INITIAL_REF",  PAR_FLOAT,    1,        1, NULL, { .f = &ccpars_ramp.initial_ref         }, 1 },
    { "FINAL_REF",    PAR_FLOAT,    1,        1, NULL, { .f = &ccpars_ramp.config.final        }, 1 },
    { "ACCELERATION", PAR_FLOAT,    1,        1, NULL, { .f = &ccpars_ramp.config.acceleration }, 1 },
    { "LINEAR_RATE",  PAR_FLOAT,    1,        1, NULL, { .f = &ccpars_ramp.config.linear_rate  }, 1 },
    { "DECELERATION", PAR_FLOAT,    1,        1, NULL, { .f = &ccpars_ramp.config.deceleration }, 1 },
    { NULL }
}
#endif
;

#endif
// EOF


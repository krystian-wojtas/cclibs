/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/func/start.h                                                     Copyright CERN 2014

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

  Purpose:  Structure for START function parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_START_H
#define CCPARS_START_H

#include "libfg.h"
#include "libfg/ramp.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_START_EXT
#else
#define CCPARS_START_EXT extern
#endif

// START data structure

struct ccpars_start
{
    // cctest START parameters

    struct fg_ramp_config       config;                 // Libfg config struct for START

    // Libfg RAMP variables

    struct fg_ramp_pars         pars;                   // Libfg parameters for START
};

CCPARS_START_EXT struct ccpars_start ccpars_start
#ifdef GLOBALS
= {// Default value         Parameter
    {
        0.0,             // config.final - this is set to LIMIT MIN and is not a parameter
        5.0,             // START ACCELERATION
        3.0,             // START LINEAR_RATE
        10.0,            // START DECELERTION
    }
}
#endif
;

// START data description structure

CCPARS_START_EXT struct ccpars   start_pars[]
#ifdef GLOBALS
= {// "Signal name"   type,     max_n_els,*enum,        *value,                       num_defaults
    { "ACCELERATION", PAR_FLOAT,    1,     NULL, { .f = &ccpars_start.config.acceleration }, 1 },
    { "LINEAR_RATE",  PAR_FLOAT,    1,     NULL, { .f = &ccpars_start.config.linear_rate  }, 1 },
    { "DECELERATION", PAR_FLOAT,    1,     NULL, { .f = &ccpars_start.config.deceleration }, 1 },
    { NULL }
}
#endif
;

#endif
// EOF

/*---------------------------------------------------------------------------------------------------------*\
  File:     func/plep.h                                                                 Copyright CERN 2014

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

  Purpose:  Structure for the plep data file (-d plep_file)

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_PLEP_H
#define CCPARS_PLEP_H

#include "ccpars.h"
#include "libfg.h"
#include "libfg/plep.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_PLEP_EXT
#else
#define CCPARS_PLEP_EXT extern
#endif

// PLEP data structure

struct ccpars_plep
{
    // PLEP file data

    float                       initial_ref;
    struct fg_plep_config       config;                  // Libfg config struct for PLEP

    // Libfg PLEP variables

    struct fg_plep_pars         plep_pars;               // Libfg parameters for PLEP
};

CCPARS_PLEP_EXT struct ccpars_plep ccpars_plep;

// PLEP data description structure

CCPARS_PLEP_EXT struct ccpars   plep_pars[]
#ifdef GLOBALS
= {// "Signal name"   TYPE, max_vals,min_vals,*enum, *value,                           num_defaults
    { "INITIAL_REF",  PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_plep.initial_ref         }, 0 },
    { "FINAL_REF",    PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_plep.config.final        }, 0 },
    { "ACCELERATION", PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_plep.config.acceleration }, 0 },
    { "LINEAR_RATE",  PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_plep.config.linear_rate  }, 0 },
    { "FINAL_RATE",   PAR_FLOAT,  1, 0, NULL,     { .f = &ccpars_plep.config.final_rate   }, 1 },  // Default = 0.0
    { "EXP_TC",       PAR_FLOAT,  1, 0, NULL,     { .f = &ccpars_plep.config.exp_tc       }, 1 },  // Default = 0.0
    { "EXP_FINAL",    PAR_FLOAT,  1, 0, NULL,     { .f = &ccpars_plep.config.exp_final    }, 1 },  // Default = 0.0
    { NULL }
}
#endif
;

#endif
// EOF


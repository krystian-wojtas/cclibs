/*---------------------------------------------------------------------------------------------------------*\
  File:     func/ramp.h                                                                Copyright CERN 2014

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

  Purpose:  Structure for the ramp data file (-d ramp_file)

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_RAMP_H
#define CCPARS_RAMP_H

#include "ccpars.h"
#include "libfg.h"
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
    struct fg_ramp_config       config;                 // Libfg config struct for RAMP

    // Libfg RAMP variables

    struct fg_ramp_pars         ramp_pars;                // Libfg parameters for RAMP
};

CCPARS_RAMP_EXT struct ccpars_ramp ccpars_ramp;

// RAMP data description structure

CCPARS_RAMP_EXT struct ccpars   ramp_pars[]
#ifdef GLOBALS
= {// "Signal name"   TYPE, max_vals,min_vals,*enum, *value,                           num_defaults
    { "INITIAL_REF",  PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_ramp.initial_ref         }, 0 },
    { "FINAL_REF",    PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_ramp.config.final        }, 0 },
    { "ACCELERATION", PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_ramp.config.acceleration }, 0 },
    { NULL }
}
#endif
;

#endif

// EOF


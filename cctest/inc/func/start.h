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
#include "libfg/plep.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_START_EXT
#else
#define CCPARS_START_EXT extern
#endif

// START data structure

struct ccpars_start
{
    // START file data

    float                       feedforward_v_ref;       // Initial voltage reference for feedforward
    float                       closeloop_level;         // Measurement threshold to close the loop
    struct fg_plep_config       config;                  // Libfg config struct for PLEP

    // Libfg PLEP (used for START) variables

    struct fg_plep_pars         plep_pars;               // Libfg parameters for PLEP
};

CCPARS_START_EXT struct ccpars_start ccpars_start;

// START data description structure

CCPARS_START_EXT struct ccpars   start_pars_list[]
#ifdef GLOBALS
= {// "Signal name"         TYPE, max_vals,min_vals,*enum,     *value,                        num_defaults
    { "FEEDFORWARD_V_REF",  PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_start.feedforward_v_ref   }, 0 },
    { "CLOSELOOP_LEVEL",    PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_start.closeloop_level     }, 0 },
    { "FINAL_REF",          PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_start.config.final        }, 0 },
    { "ACCELERATION",       PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_start.config.acceleration }, 0 },
    { NULL }
}
#endif
;

#endif
// EOF

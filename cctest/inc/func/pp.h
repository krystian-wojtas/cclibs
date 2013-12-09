/*---------------------------------------------------------------------------------------------------------*\
  File:     func/pp.h                                                                 Copyright CERN 2014

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

  Purpose:  Structure for the pp data file (-d pp_file)

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_PP_H
#define CCPARS_PP_H

#include "ccpars.h"
#include "libfg.h"
#include "libfg/pp.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_PP_EXT
#else
#define CCPARS_PP_EXT extern
#endif

// PP data structure

struct ccpars_pp
{
    // PP file data

    float                       initial_ref;
    struct fg_pp_config         config;                 // Libfg config struct for PP

    // Libfg PP variables

    struct fg_pp_pars           pp_pars;                // Libfg parameters for PP
};

CCPARS_PP_EXT struct ccpars_pp ccpars_pp;

// PP data description structure

CCPARS_PP_EXT struct ccpars   pp_pars[]
#ifdef GLOBALS
= {// "Signal name"   TYPE, max_vals,min_vals,*enum, *value,                           num_defaults
    { "INITIAL_REF",  PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_pp.initial_ref         }, 0 },
    { "FINAL_REF",    PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_pp.config.final        }, 0 },
    { "ACCELERATION", PAR_FLOAT,  1, 1, NULL,     { .f = &ccpars_pp.config.acceleration }, 0 },
    { NULL }
}
#endif
;

#endif

// EOF


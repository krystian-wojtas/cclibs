/*---------------------------------------------------------------------------------------------------------*\
  File:     func/pppl.h                                                                 Copyright CERN 2011

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

  Purpose:  Structure for the PPPL data file (-d pppl_file)

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_PPPL_H
#define CCPARS_PPPL_H

#include "libfg/pppl.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_PPPL_EXT
#else
#define CCPARS_PPPL_EXT extern
#endif

// PPPL parameters structure

struct ccpars_pppl
{
    // PPPL file data

    float                       initial_ref;
    struct fg_pppl_config       config;                 // Use Libfg for configuration data

    // Libfg pppl variables

    struct fg_pppl_pars         pppl_pars;             // Libfg parameters for PPPL
};

CCPARS_PPPL_EXT struct ccpars_pppl ccpars_pppl;

// PPPL data description structure

CCPARS_PPPL_EXT struct ccpars   pppl_pars_list[]
#ifdef GLOBALS
= {// "Signal name"    TYPE,   max_vals,   min_vals, *enum, *value,                           num_defaults
    { "INITIAL_REF",   PAR_FLOAT, 1,            1, NULL, { .f = &ccpars_pppl.initial_ref          }, 0 },
    { "ACCELERATION1", PAR_FLOAT, FG_MAX_PPPLS, 1, NULL, { .f =  ccpars_pppl.config.acceleration1 }, 0 },
    { "ACCELERATION2", PAR_FLOAT, FG_MAX_PPPLS, 1, NULL, { .f =  ccpars_pppl.config.acceleration2 }, 0 },
    { "ACCELERATION3", PAR_FLOAT, FG_MAX_PPPLS, 1, NULL, { .f =  ccpars_pppl.config.acceleration3 }, 0 },
    { "RATE2",         PAR_FLOAT, FG_MAX_PPPLS, 1, NULL, { .f =  ccpars_pppl.config.rate2         }, 0 },
    { "RATE4",         PAR_FLOAT, FG_MAX_PPPLS, 1, NULL, { .f =  ccpars_pppl.config.rate4         }, 0 },
    { "REF4",          PAR_FLOAT, FG_MAX_PPPLS, 1, NULL, { .f =  ccpars_pppl.config.ref4          }, 0 },
    { "DURATION4",     PAR_FLOAT, FG_MAX_PPPLS, 1, NULL, { .f =  ccpars_pppl.config.duration4     }, 0 },
    { NULL }
}
#endif
;

#endif
/*---------------------------------------------------------------------------------------------------------*\
  End of file: func/pppl.h
\*---------------------------------------------------------------------------------------------------------*/


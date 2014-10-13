/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/func/pppl.h                                                      Copyright CERN 2014

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

  Purpose:  Structure for PPPL function parameters

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
    // cctest PPPL parameters

    float                       initial_ref;
    struct fg_pppl_config       config;                 // Libfg config struct for PPPL

    // Libfg PPPL variables

    struct fg_pppl_pars         pars;                   // Libfg parameters for PPPL
};

CCPARS_PPPL_EXT struct ccpars_pppl ccpars_pppl
#ifdef GLOBALS
= {//   Default value           Parameter
        0.0,                 // PPPL INITIAL_REF
        {
            {  5.0 },        // PLEP ACCELERATION1
            { -0.1 },        // PLEP ACCELERATION2
            { -2.0 },        // PLEP ACCELERATION3
            {  1.0 },        // PLEP RATE2
            {  0.0 },        // PLEP RATE4
            {  1.0 },        // PLEP REF4
            {  0.1 },        // PLEP DURATION4
        }
}
#endif
;

// PPPL data description structure

CCPARS_PPPL_EXT struct ccpars   pppl_pars[]
#ifdef GLOBALS
= {// "Signal name"    type,         max_n_els, *enum,        *value,                        num_defaults
    { "INITIAL_REF",   PAR_FLOAT,            1,  NULL, { .f = &ccpars_pppl.initial_ref          }, 1 },
    { "ACCELERATION1", PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl.config.acceleration1 }, 1 },
    { "ACCELERATION2", PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl.config.acceleration2 }, 1 },
    { "ACCELERATION3", PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl.config.acceleration3 }, 1 },
    { "RATE2",         PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl.config.rate2         }, 1 },
    { "RATE4",         PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl.config.rate4         }, 1 },
    { "REF4",          PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl.config.ref4          }, 1 },
    { "DURATION4",     PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl.config.duration4     }, 1 },
    { NULL }
}
#endif
;

#endif
// EOF

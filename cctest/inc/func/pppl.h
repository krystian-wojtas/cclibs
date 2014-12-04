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

#include "ccTest.h"
#include "ccPars.h"
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

    float                       initial_ref  [CC_NUM_CYC_SELS][1];                // Initial reference
    float                       acceleration1[CC_NUM_CYC_SELS][FG_MAX_PPPLS];     // Acceleration of first  (parabolic) segment.
    float                       acceleration2[CC_NUM_CYC_SELS][FG_MAX_PPPLS];     // Acceleration of second (parabolic) segment.
    float                       acceleration3[CC_NUM_CYC_SELS][FG_MAX_PPPLS];     // Acceleration of third  (parabolic) segment.
    float                       rate2        [CC_NUM_CYC_SELS][FG_MAX_PPPLS];     // Rate of change at start of second (parabolic) segment.
    float                       rate4        [CC_NUM_CYC_SELS][FG_MAX_PPPLS];     // Rate of change of fourth (linear) segment.
    float                       ref4         [CC_NUM_CYC_SELS][FG_MAX_PPPLS];     // Reference at start of fourth (linear) segment.
    float                       duration4    [CC_NUM_CYC_SELS][FG_MAX_PPPLS];     // Duration of fourth (linear) segment.

    // Libfg PPPL variables

    struct fg_pppl_pars         pars[CC_NUM_CYC_SELS];                            // Libfg parameters for PPPL
};

CCPARS_PPPL_EXT struct ccpars_pppl ccpars_pppl
#ifdef GLOBALS
= {//   Default value           Parameter
    { {  0.0 } },            // PPPL INITIAL_REF
    { {  5.0 } },            // PLEP ACCELERATION1
    { { -0.1 } },            // PLEP ACCELERATION2
    { { -2.0 } },            // PLEP ACCELERATION3
    { {  1.0 } },            // PLEP RATE2
    { {  0.0 } },            // PLEP RATE4
    { {  1.0 } },            // PLEP REF4
    { {  0.1 } },            // PLEP DURATION4
}
#endif
;

// PPPL data description structure

CCPARS_PPPL_EXT struct ccpars   pppl_pars[]
#ifdef GLOBALS
= {// "Signal name"    type,         max_n_els, *enum,        *value,                       num_defaults  flags
    { "INITIAL_REF",   PAR_FLOAT,            1,  NULL, { .f = &ccpars_pppl.initial_ref  [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "ACCELERATION1", PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f = &ccpars_pppl.acceleration1[0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "ACCELERATION2", PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f = &ccpars_pppl.acceleration2[0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "ACCELERATION3", PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f = &ccpars_pppl.acceleration3[0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "RATE2",         PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f = &ccpars_pppl.rate2        [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "RATE4",         PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f = &ccpars_pppl.rate4        [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "REF4",          PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f = &ccpars_pppl.ref4         [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "DURATION4",     PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f = &ccpars_pppl.duration4    [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { NULL }
}
#endif
;

#endif
// EOF

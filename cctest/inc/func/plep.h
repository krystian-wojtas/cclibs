/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/func/plep.h                                                      Copyright CERN 2014

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

  Purpose:  Structure for PLEP function parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_PLEP_H
#define CCPARS_PLEP_H

#include "ccTest.h"
#include "ccPars.h"
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
    // cctest PLEP parameters

    float                       initial_ref [CC_NUM_CYC_SELS][1];     // Initial reference
    float                       final       [CC_NUM_CYC_SELS][1];     // Final reference
    float                       acceleration[CC_NUM_CYC_SELS][1];     // Acceleration of the parabolic segments (absolute value is used)
    float                       linear_rate [CC_NUM_CYC_SELS][1];     // Maximum linear rate (absolute value is used)
    float                       final_rate  [CC_NUM_CYC_SELS][1];     // Final rate of change
    float                       exp_tc      [CC_NUM_CYC_SELS][1];     // Exponential time constant
    float                       exp_final   [CC_NUM_CYC_SELS][1];     // End reference of exponential segment (can be zero)

    // Libfg PLEP variables

    struct fg_plep_pars         pars[CC_NUM_CYC_SELS];                // Libfg parameters for PLEP
};

CCPARS_PLEP_EXT struct ccpars_plep ccpars_plep
#ifdef GLOBALS
= {// Default value           Parameter
    { { 0.0 } },           // PLEP INITIAL_REF
    { { 1.0 } },           // PLEP FINAL_REF
    { { 1.0 } },           // PLEP ACCELERATION
    { { 1.0 } },           // PLEP LINEAR_RATE
    { { 0.0 } },           // PLEP FINAL_RATE
    { { 0.0 } },           // PLEP EXP_TC
    { { 0.0 } },           // PLEP EXP_FINAL
}
#endif
;

// PLEP data description structure

CCPARS_PLEP_EXT struct ccpars   plep_pars[]
#ifdef GLOBALS
= {// "Signal name"   type,     max_n_els,*enum,        *value,                      num_defaults  flags
    { "INITIAL_REF",  PAR_FLOAT,    1,     NULL, { .f = &ccpars_plep.initial_ref [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "FINAL_REF",    PAR_FLOAT,    1,     NULL, { .f = &ccpars_plep.final       [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "ACCELERATION", PAR_FLOAT,    1,     NULL, { .f = &ccpars_plep.acceleration[0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "LINEAR_RATE",  PAR_FLOAT,    1,     NULL, { .f = &ccpars_plep.linear_rate [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "FINAL_RATE",   PAR_FLOAT,    1,     NULL, { .f = &ccpars_plep.final_rate  [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "EXP_TC",       PAR_FLOAT,    1,     NULL, { .f = &ccpars_plep.exp_tc      [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "EXP_FINAL",    PAR_FLOAT,    1,     NULL, { .f = &ccpars_plep.exp_final   [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { NULL }
}
#endif
;

#endif
// EOF


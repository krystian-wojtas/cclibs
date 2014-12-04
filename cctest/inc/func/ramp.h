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

#include "ccTest.h"
#include "ccPars.h"
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
    // cctest RAMP parameters

    float                       initial_ref [CC_NUM_CYC_SELS][1];     // Initial reference
    float                       final       [CC_NUM_CYC_SELS][1];     // Final reference
    float                       acceleration[CC_NUM_CYC_SELS][1];     // Acceleration of the 1st parabolic segment. Absolute value is used.
    float                       linear_rate [CC_NUM_CYC_SELS][1];     // Maximum linear rate. Absolute value is used.
    float                       deceleration[CC_NUM_CYC_SELS][1];     // Deceleration of the 2nd parabolic segment. Absolute value is used.

    // Libfg RAMP variables

    struct fg_ramp_pars         pars[CC_NUM_CYC_SELS];                // Libfg parameters for RAMP
};

CCPARS_RAMP_EXT struct ccpars_ramp ccpars_ramp
#ifdef GLOBALS
= {// Default value             Parameter
    { { 0.0 } },             // RAMP INITIAL_REF
    { { 1.0 } },             // RAMP FINAL_REF
    { { 4.0 } },             // RAMP ACCELERATION
    { { 1.0 } },             // RAMP LINEAR_RATE
    { { 6.0 } },             // RAMP DECELERTION
}
#endif
;

// RAMP data description structure

CCPARS_RAMP_EXT struct ccpars   ramp_pars[]
#ifdef GLOBALS
= {// "Signal name"   type,     max_n_els,*enum,        *value,                      num_defaults  flags
    { "INITIAL_REF",  PAR_FLOAT,    1,     NULL, { .f = &ccpars_ramp.initial_ref [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "FINAL_REF",    PAR_FLOAT,    1,     NULL, { .f = &ccpars_ramp.final       [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "ACCELERATION", PAR_FLOAT,    1,     NULL, { .f = &ccpars_ramp.acceleration[0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "LINEAR_RATE",  PAR_FLOAT,    1,     NULL, { .f = &ccpars_ramp.linear_rate [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "DECELERATION", PAR_FLOAT,    1,     NULL, { .f = &ccpars_ramp.deceleration[0][0] }, 1, PARS_CYCLE_SELECTOR },
    { NULL }
}
#endif
;

#endif
// EOF


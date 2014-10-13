/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/func/test.h                                                      Copyright CERN 2014

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

  Purpose:  Structure for test function (SINE, COSINE, STEPS, SQUARE) parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_TEST_H
#define CCPARS_TEST_H

#include "libfg/test.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_TEST_EXT
#else
#define CCPARS_TEST_EXT extern
#endif

// Test parameters structure

struct ccpars_test
{
    // cctest TEST parameters

    float                       initial_ref;            // Initial reference
    struct fg_test_config       config;                 // Libfg config struct for TEST

    // Libfg TEST variables

    struct fg_test_pars         pars;                   // Libfg parameters for TEST
};

CCPARS_TEST_EXT struct ccpars_test ccpars_test
#ifdef GLOBALS
= {//   Default value           Parameter
        0.0,                 // TEST INITIAL_REF
        {
            FG_TEST_COSINE,  // Overwritten by init function (SINE, COSINE, STEPS or SQUARE)
            2.0,             // TEST AMPLITUDE_PP
            3.0,             // TEST NUM_CYCLES
            2.0,             // TEST PERIOD
            REG_ENABLED      // TEST WINDOW
        }
}
#endif
;

// Test data description structure

CCPARS_TEST_EXT struct ccpars test_pars[]
#ifdef GLOBALS
= {// "Signal name"   type,     max_n_els,*enum,                   *value,                       num_defaults
    { "INITIAL_REF",  PAR_FLOAT,    1,     NULL,             { .f = &ccpars_test.initial_ref         }, 1 },
    { "AMPLITUDE_PP", PAR_FLOAT,    1,     NULL,             { .f = &ccpars_test.config.amplitude_pp }, 1 },
    { "NUM_CYCLES",   PAR_FLOAT,    1,     NULL,             { .f = &ccpars_test.config.num_cycles   }, 1 },
    { "PERIOD",       PAR_FLOAT,    1,     NULL,             { .f = &ccpars_test.config.period       }, 1 },
    { "WINDOW",       PAR_ENUM,     1,     enabled_disabled, { .b = &ccpars_test.config.use_window   }, 1 },
    { NULL }
}
#endif
;

#endif
// EOF

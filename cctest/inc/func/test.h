/*---------------------------------------------------------------------------------------------------------*\
  File:     func/test.h                                                                 Copyright CERN 2011

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

  Purpose:  Structure for the test data file (-d test_file)

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_TEST_H
#define CCPARS_TEST_H

#include "ccpars.h"
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
    // Test file data

    float                      initial_ref;
    struct fg_test_config      config;                  // Libfg config struct for TEST

    // Libfg test variables

    struct fg_test_pars        test_pars;               // Libfg parameters for TEST
};

CCPARS_TEST_EXT struct ccpars_test ccpars_test;         // Default window is disabled

// Test data description structure

CCPARS_TEST_EXT struct ccpars   test_pars[]
#ifdef GLOBALS
= {// "Signal name"   TYPE, max_vals,min_vals,*enum,         *value,                           num_defaults
    { "INITIAL_REF",  PAR_FLOAT,   1, 1, NULL,             { .f = &ccpars_test.initial_ref         }, 0 },
    { "AMPLITUDE_PP", PAR_FLOAT,   1, 1, NULL,             { .f = &ccpars_test.config.amplitude_pp }, 0 },
    { "NUM_CYCLES",   PAR_FLOAT,   1, 1, NULL,             { .f = &ccpars_test.config.num_cycles   }, 0 },
    { "PERIOD",       PAR_FLOAT,   1, 1, NULL,             { .f = &ccpars_test.config.period       }, 0 },
    { "WINDOW",       PAR_ENUM,    1, 0, enabled_disabled, { .i = &ccpars_test.config.window_flag  }, 1 },
    { NULL }
}
#endif
;

#endif
/*---------------------------------------------------------------------------------------------------------*\
  End of file: func/test.h
\*---------------------------------------------------------------------------------------------------------*/


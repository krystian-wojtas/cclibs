/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/func/trim.h                                                      Copyright CERN 2014

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

  Purpose:  Structure for trim function (CTRIM, LTRIM) parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_TRIM_H
#define CCPARS_TRIM_H

#include "libfg/trim.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_TRIM_EXT
#else
#define CCPARS_TRIM_EXT extern
#endif

// Trim parameters structure

struct ccpars_trim
{
    // cctest TRIM parameters

    float                       initial_ref;            // Initial reference
    struct fg_trim_config       config;                 // Libfg config struct for TRIM

    // Libfg TRIM variables

    struct fg_trim_pars         pars;                   // Libfg parameters for TRIM
};

CCPARS_TRIM_EXT struct ccpars_trim ccpars_trim
#ifdef GLOBALS
= {//   Default value           Parameter
        0.0,                 // TRIM INITIAL_REF
        {
            FG_TRIM_LINEAR,  // Overwritten by init function (LTRIM or CTRIM)
            1.0,             // TRIM DURATION
            1.0,             // TRIM FINAL
        }
}
#endif
;

// Trim data description structure

CCPARS_TRIM_EXT struct ccpars   trim_pars[]
#ifdef GLOBALS
= {// "Signal name"  type,max_n_els,min_n_els,*enum,        *value,                   num_defaults
    { "INITIAL_REF", PAR_FLOAT,   1,        1, NULL, { .f = &ccpars_trim.initial_ref     }, 1 },
    { "FINAL_REF",   PAR_FLOAT,   1,        1, NULL, { .f = &ccpars_trim.config.final    }, 1 },
    { "DURATION",    PAR_FLOAT,   1,        1, NULL, { .f = &ccpars_trim.config.duration }, 1 },
    { NULL }
}
#endif
;

#endif
// EOF

/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/pars/limits.h                                                    Copyright CERN 2014

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

  Purpose:  Structure for the limit parameters file (-m limits_file)

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_LIMITS_H
#define CCPARS_LIMITS_H

#include "cccmds.h"
#include "libfg.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_LIMITS_EXT
#else
#define CCPARS_LIMITS_EXT extern
#endif

// Limit parameters structure

struct ccpars_limits
{
    // Limits file parameters

    struct fg_limits    b;                      // Field limits
    float               b_err_warning;          // Field regulation error warning limit
    float               b_err_fault;            // Field regulation error fault limit

    struct fg_limits    i;                      // Current limits
    float               i_err_warning;          // Current regulation error warning limit
    float               i_err_fault;            // Current regulation error fault limit
    float               i_quadrants41[2];       // Exclusion zone for quadrants 4 and 1

    struct fg_limits    v;                      // Voltage limits
    float               v_err_warning;          // Current regulation error warning limit
    float               v_err_fault;            // Current regulation error fault limit
    float               v_quadrants41[2];

    uint32_t            invert_limits;          // Invert real-time limits (switch is negative)

    // Limits related variables

    struct fg_limits   *fg;                     // Pointer to fg_limits (b/i/v)
    struct reg_lim_ref  fg_v_ref;               // Libreg voltage measurement limits structure
};

CCPARS_LIMITS_EXT struct ccpars_limits ccpars_limits
#ifdef GLOBALS
= {//  POS   MIN    NEG    RATE   ACC   ERR_WRN ERR_FLT  Quadrant41 
    { 1.0E1, 0.0, -1.0E1, 1.0E1, 1.0E6 }, 0.0,   0.0,                 // LIMITS.B_limits
    { 1.0E1, 0.0, -1.0E1, 1.0E1, 1.0E6 }, 0.0,   0.0,  { 0.0, 0.0 },  // LIMITS.I_limits
    { 1.0E1, 0.0, -1.0E1, 1.0E1, 1.0E6 }, 0.0,   0.0,  { 0.0, 0.0 },  // LIMITS.V_limits
    CC_DISABLED                                                       // LIMITS.INVERT_LIMITS
}
#endif
;

// Global parameters description structure

CCPARS_LIMITS_EXT struct ccpars limits_pars[]
#ifdef GLOBALS
= {// "Signal name"     TYPE,   max_vals,min_vals,*enum,          *value,                    num_defaults
    { "B_POS",          PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.b.pos          }, 1 },
    { "B_MIN",          PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.b.min          }, 1 },
    { "B_NEG",          PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.b.neg          }, 1 },
    { "B_RATE",         PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.b.rate         }, 1 },
    { "B_ACCELERATION", PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.b.acceleration }, 1 },
    { "B_ERR_WARNING",  PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.b_err_warning  }, 1 },
    { "B_ERR_FAULT",    PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.b_err_fault    }, 1 },
    { "I_POS",          PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.i.pos          }, 1 },
    { "I_MIN",          PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.i.min          }, 1 },
    { "I_NEG",          PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.i.neg          }, 1 },
    { "I_RATE",         PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.i.rate         }, 1 },
    { "I_ACCELERATION", PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.i.acceleration }, 1 },
    { "I_ERR_WARNING",  PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.i_err_warning  }, 1 },
    { "I_ERR_FAULT",    PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.i_err_fault    }, 1 },
    { "I_QUADRANTS41",  PAR_FLOAT, 2, 1, NULL,             { .f =  ccpars_limits.i_quadrants41  }, 2 },
    { "V_POS",          PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.v.pos          }, 1 },
    { "V_NEG",          PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.v.neg          }, 1 },
    { "V_RATE",         PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.v.rate         }, 1 },
    { "V_ACCELERATION", PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.v.acceleration }, 1 },
    { "V_ERR_WARNING",  PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.v_err_warning  }, 1 },
    { "V_ERR_FAULT",    PAR_FLOAT, 1, 1, NULL,             { .f = &ccpars_limits.v_err_fault    }, 1 },
    { "V_QUADRANTS41",  PAR_FLOAT, 2, 1, NULL,             { .f =  ccpars_limits.v_quadrants41  }, 2 },
    { "INVERT_LIMITS",  PAR_ENUM,  1, 1, enabled_disabled, { .i = &ccpars_limits.invert_limits  }, 1 },
    { NULL }
}
#endif
;

#endif
// EOF

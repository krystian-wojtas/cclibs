/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/pars/reg.h                                                       Copyright CERN 2014

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

  Purpose:  Structure for the regulation parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_REG_H
#define CCPARS_REG_H

#include "cccmds.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_REG_EXT
#else
#define CCPARS_REG_EXT extern
#endif

// Field and current Regulation parameters

struct ccpars_reg_pars
{
    uint32_t            period_iters;               // Regulation period in iteration periods
    float               track_delay_periods;        // Regulation track delay in periods
    float               clbw;                       // Regulation closed loop bandwidth (real pole)
    float               clbw2;                      // Regulation closed loop bandwidth (conjugate poles)
    float               z;                          // Regulation conjugate poles damping factor (0.5-0.8)
    float               clbw3;                      // Regulation closed loop bandwidth (second real pole)
    float               clbw4;                      // Regulation closed loop bandwidth (third real pole)
    struct reg_rst      rst;                        // Regulation RST coefficient
};

CCPARS_REG_EXT struct ccpars_reg_pars ccpars_breg
#ifdef GLOBALS
= {//   Default value           Parameter
        10,                  // BREG.PERIOD_ITERS
        1.0,                 // BREG.TRACK_DELAY_PERIODS
        1.0,                 // BREG.CLBW
        1.0,                 // BREG.CLBW2
        0.5,                 // BREG.Z
        1.0,                 // BREG.CLBW3
        1.0,                 // BREG.CLBW4
        { {  0.0  },         // BREG.R
          {  0.0  },         // BREG.S
          {  0.0  } },       // BREG.T
}
#endif
;

CCPARS_REG_EXT struct ccpars_reg_pars ccpars_ireg
#ifdef GLOBALS
= {//   Default value           Parameter
        10,                  // IREG.PERIOD_ITERS
        1.0,                 // IREG.TRACK_DELAY_PERIODS
        1.0,                 // IREG.CLBW
        1.0,                 // IREG.CLBW2
        0.5,                 // IREG.Z
        1.0,                 // IREG.CLBW3
        1.0,                 // IREG.CLBW4
        { {  0.0  },         // IREG.R
          {  0.0  },         // IREG.S
          {  0.0  } },       // IREG.T
}
#endif
;

// Libreg structures

CCPARS_REG_EXT struct reg_converter reg;            // Libreg converter regulation structure
CCPARS_REG_EXT struct reg_converter_pars reg_pars;  // Libreg converter regulation parameters structure

// Define Field and Current regulation parameters description structures

CCPARS_REG_EXT struct ccpars breg_pars[]
#ifdef GLOBALS
= {// "Signal name"         TYPE,         max_vals,  min_vals,*enum,          *value,                             num_defaults
    { "PERIOD_ITERS",       PAR_UNSIGNED, 1,                1, NULL,        { .i = &ccpars_reg_b.period_iters        }, 1 },
    { "TRACK_DELAY_PERIODS",PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_reg_b.track_delay_periods }, 1 },
    { "CLBW",               PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_reg_b.clbw                }, 1 },
    { "CLBW2",              PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_reg_b.clbw2               }, 1 },
    { "Z",                  PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_reg_b.z                   }, 1 },
    { "CLBW3",              PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_reg_b.clbw3               }, 1 },
    { "CLBW4",              PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_reg_b.clbw4               }, 1 },
    { "R",                  PAR_FLOAT,    REG_N_RST_COEFFS, 0, NULL,        { .f =  ccpars_reg_b.rst.r               }, 0 },
    { "S",                  PAR_FLOAT,    REG_N_RST_COEFFS, 0, NULL,        { .f =  ccpars_reg_b.rst.s               }, 0 },
    { "T",                  PAR_FLOAT,    REG_N_RST_COEFFS, 0, NULL,        { .f =  ccpars_reg_b.rst.t               }, 0 },
    { NULL }
}
#endif
;

CCPARS_REG_EXT struct ccpars ireg_pars[]
#ifdef GLOBALS
= {// "Signal name"         TYPE,         max_vals,  min_vals,*enum,          *value,                             num_defaults
    { "PERIOD_ITERS",       PAR_UNSIGNED, 1,                1, NULL,        { .i = &ccpars_reg_i.period_iters        }, 1 },
    { "TRACK_DELAY_PERIODS",PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_reg_i.track_delay_periods }, 1 },
    { "CLBW",               PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_reg_i.clbw                }, 1 },
    { "CLBW2",              PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_reg_i.clbw2               }, 1 },
    { "Z",                  PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_reg_i.z                   }, 1 },
    { "CLBW3",              PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_reg_i.clbw3               }, 1 },
    { "CLBW4",              PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_reg_i.clbw4               }, 1 },
    { "R",                  PAR_FLOAT,    REG_N_RST_COEFFS, 0, NULL,        { .f =  ccpars_reg_i.rst.r               }, 0 },
    { "S",                  PAR_FLOAT,    REG_N_RST_COEFFS, 0, NULL,        { .f =  ccpars_reg_i.rst.s               }, 0 },
    { "T",                  PAR_FLOAT,    REG_N_RST_COEFFS, 0, NULL,        { .f =  ccpars_reg_i.rst.t               }, 0 },
    { NULL }
}
#endif
;

#endif
// EOF

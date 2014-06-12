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

#include "ccCmds.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_REG_EXT
#else
#define CCPARS_REG_EXT extern
#endif

// Error rate control enum

CCPARS_EXT struct ccpars_enum err_rate[]
#ifdef GLOBALS
= {
    { REG_ERR_RATE_REGULATION,   "REGULATION"     },
    { REG_ERR_RATE_MEASUREMENT,  "MEASUREMENT"    },
    { 0,                          NULL            },
}
#endif
;

// Field and current Regulation parameters

struct ccpars_reg_pars
{
    uint32_t            period_iters;               // Regulation period in iteration periods
    enum reg_err_rate   err_rate;                   // Regulation error rate control
    float               pure_delay_periods;         // Regulation pure delay in periods (0 to use automatic calculation)
    float               track_delay_periods;        // Regulation track delay in periods (0 to use automatic calculation)
    float               clbw;                       // Regulation closed loop bandwidth (real pole)
    float               clbw2;                      // Regulation closed loop bandwidth (conjugate poles)
    float               clbw3;                      // Regulation closed loop bandwidth (2nd real pole)
    float               clbw4;                      // Regulation closed loop bandwidth (3rd real pole)
    float               z;                          // Regulation conjugate poles damping factor (0.5-0.8)
    struct reg_rst      rst;                        // Regulation RST coefficient
};

CCPARS_REG_EXT struct ccpars_reg_pars ccpars_breg
#ifdef GLOBALS
= {//   Default value                  Parameter
        10,                         // BREG PERIOD_ITERS
        REG_ERR_RATE_REGULATION,    // BREG ERR_RATE
        0.0,                        // BREG PURE_DELAY_PERIODS
        0.0,                        // BREG TRACK_DELAY_PERIODS
        10.0,                       // BREG CLBW
        10.0,                       // BREG CLBW2
        10.0,                       // BREG CLBW3
        10.0,                       // BREG CLBW4
        0.5,                        // BREG Z
        { {  0.0  },                // BREG R
          {  0.0  },                // BREG S
          {  0.0  } },              // BREG T
}
#endif
;

CCPARS_REG_EXT struct ccpars_reg_pars ccpars_ireg
#ifdef GLOBALS
= {//   Default value                  Parameter
        10,                         // IREG PERIOD_ITERS
        REG_ERR_RATE_REGULATION,    // IREG ERR_RATE
        0.0,                        // IREG PURE_DELAY_PERIODS
        0.0,                        // IREG TRACK_DELAY_PERIODS
        10.0,                       // IREG CLBW
        10.0,                       // IREG CLBW2
        10.0,                       // IREG CLBW3
        10.0,                       // IREG CLBW4
        0.5,                        // IREG Z
        { {  0.0  },                // IREG R
          {  0.0  },                // IREG S
          {  0.0  } },              // IREG T
}
#endif
;

// Libreg structures

CCPARS_REG_EXT struct reg_conv reg;            // Libreg converter regulation structure

// Define Field and Current regulation parameters description structures

CCPARS_REG_EXT struct ccpars breg_pars[]
#ifdef GLOBALS
= {// "Signal name"         type,         max_n_els,  min_n_els,*enum,          *value,                            num_defaults
    { "PERIOD_ITERS",       PAR_UNSIGNED, 1,                1, NULL,        { .i = &ccpars_breg.period_iters        }, 1 },
    { "ERR_RATE",           PAR_ENUM,     1,                1, err_rate,    { .i = &ccpars_breg.err_rate            }, 1 },
    { "PURE_DELAY_PERIODS", PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.pure_delay_periods  }, 1 },
    { "TRACK_DELAY_PERIODS",PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.track_delay_periods }, 1 },
    { "CLBW",               PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.clbw                }, 1 },
    { "CLBW2",              PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.clbw2               }, 1 },
    { "CLBW3",              PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.clbw3               }, 1 },
    { "CLBW4",              PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.clbw4               }, 1 },
    { "Z",                  PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.z                   }, 1 },
    { "R",                  PAR_DOUBLE,   REG_N_RST_COEFFS, 0, NULL,        { .d =  ccpars_breg.rst.r               }, 0 },
    { "S",                  PAR_DOUBLE,   REG_N_RST_COEFFS, 0, NULL,        { .d =  ccpars_breg.rst.s               }, 0 },
    { "T",                  PAR_DOUBLE,   REG_N_RST_COEFFS, 0, NULL,        { .d =  ccpars_breg.rst.t               }, 0 },
    { NULL }
}
#endif
;

CCPARS_REG_EXT struct ccpars ireg_pars[]
#ifdef GLOBALS
= {// "Signal name"         type,         max_n_els,  min_n_els,*enum,          *value,                            num_defaults
    { "PERIOD_ITERS",       PAR_UNSIGNED, 1,                1, NULL,        { .i = &ccpars_ireg.period_iters        }, 1 },
    { "ERR_RATE",           PAR_ENUM,     1,                1, err_rate,    { .i = &ccpars_ireg.err_rate            }, 1 },
    { "PURE_DELAY_PERIODS", PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.pure_delay_periods  }, 1 },
    { "TRACK_DELAY_PERIODS",PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.track_delay_periods }, 1 },
    { "CLBW",               PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.clbw                }, 1 },
    { "CLBW2",              PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.clbw2               }, 1 },
    { "CLBW3",              PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.clbw3               }, 1 },
    { "CLBW4",              PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.clbw4               }, 1 },
    { "Z",                  PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.z                   }, 1 },
    { "R",                  PAR_DOUBLE,   REG_N_RST_COEFFS, 0, NULL,        { .d =  ccpars_ireg.rst.r               }, 0 },
    { "S",                  PAR_DOUBLE,   REG_N_RST_COEFFS, 0, NULL,        { .d =  ccpars_ireg.rst.s               }, 0 },
    { "T",                  PAR_DOUBLE,   REG_N_RST_COEFFS, 0, NULL,        { .d =  ccpars_ireg.rst.t               }, 0 },
    { NULL }
}
#endif
;

#endif
// EOF

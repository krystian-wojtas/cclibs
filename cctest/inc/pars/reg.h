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

// Field and current Regulation parameters

struct ccpars_reg_pars
{
    uint32_t            period_iters;               // Regulation period in iteration periods
    float               pure_delay_periods;         // Regulation pure delay in periods (0 to use automatic calculation)
    float               track_delay_periods;        // Regulation track delay in periods (0 to use automatic calculation)
    float               auxpole1_hz;                // Frequency of (real) auxiliary pole 1
    float               auxpoles2_hz;               // Frequency of (conjugate) auxiliary poles 2 & 3
    float               auxpoles2_z;                // Damping of (conjugate) auxiliary poles 2 & 3
    float               auxpole4_hz;                // Frequency of (real) auxiliary pole 4
    float               auxpole5_hz;                // Frequency of (real) auxiliary pole 5
    struct reg_rst      rst;                        // Regulation RST coefficient
};

CCPARS_REG_EXT struct ccpars_reg_pars ccpars_breg
#ifdef GLOBALS
= {//   Default value                  Parameter
        10,                         // BREG PERIOD_ITERS
        0.0,                        // BREG PURE_DELAY_PERIODS
        0.0,                        // BREG TRACK_DELAY_PERIODS
        10.0,                       // BREG AUXPOLE1_HZ
        10.0,                       // BREG AUXPOLES2_HZ
        0.5,                        // BREG AUXPOLES2_Z
        10.0,                       // BREG AUXPOLE4_HZ
        10.0,                       // BREG AUXPOLE5_HZ
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
        0.0,                        // IREG PURE_DELAY_PERIODS
        0.0,                        // IREG TRACK_DELAY_PERIODS
        10.0,                       // IREG AUXPOLE1_HZ
        10.0,                       // IREG AUXPOLES2_HZ
        0.5,                        // IREG AUXPOLES2_Z
        10.0,                       // IREG AUXPOLE4_HZ
        10.0,                       // IREG AUXPOLE5_HZ
        { {  0.0  },                // IREG R
          {  0.0  },                // IREG S
          {  0.0  } },              // IREG T
}
#endif
;

// Libreg structures

CCPARS_REG_EXT struct reg_conv conv;            // Libreg converter regulation structure

// Define Field and Current regulation parameters description structures

CCPARS_REG_EXT struct ccpars breg_pars[]
#ifdef GLOBALS
= {// "Signal name"         type,         max_n_els,  min_n_els,*enum,          *value,                            num_defaults
    { "PERIOD_ITERS",       PAR_UNSIGNED, 1,                1, NULL,        { .i = &ccpars_breg.period_iters        }, 1 },
    { "PURE_DELAY_PERIODS", PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.pure_delay_periods  }, 1 },
    { "TRACK_DELAY_PERIODS",PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.track_delay_periods }, 1 },
    { "AUXPOLE1_HZ",        PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.auxpole1_hz         }, 1 },
    { "AUXPOLES2_HZ",       PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.auxpoles2_hz        }, 1 },
    { "AUXPOLES2_Z",        PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.auxpoles2_z         }, 1 },
    { "AUXPOLE4_HZ",        PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.auxpole4_hz         }, 1 },
    { "AUXPOLE5_HZ",        PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_breg.auxpole5_hz         }, 1 },
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
    { "PURE_DELAY_PERIODS", PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.pure_delay_periods  }, 1 },
    { "TRACK_DELAY_PERIODS",PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.track_delay_periods }, 1 },
    { "AUXPOLE1_HZ",        PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.auxpole1_hz         }, 1 },
    { "AUXPOLES2_HZ",       PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.auxpoles2_hz        }, 1 },
    { "AUXPOLES2_Z",        PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.auxpoles2_z         }, 1 },
    { "AUXPOLE4_HZ",        PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.auxpole4_hz         }, 1 },
    { "AUXPOLE5_HZ",        PAR_FLOAT,    1,                1, NULL,        { .f = &ccpars_ireg.auxpole5_hz         }, 1 },
    { "R",                  PAR_DOUBLE,   REG_N_RST_COEFFS, 0, NULL,        { .d =  ccpars_ireg.rst.r               }, 0 },
    { "S",                  PAR_DOUBLE,   REG_N_RST_COEFFS, 0, NULL,        { .d =  ccpars_ireg.rst.s               }, 0 },
    { "T",                  PAR_DOUBLE,   REG_N_RST_COEFFS, 0, NULL,        { .d =  ccpars_ireg.rst.t               }, 0 },
    { NULL }
}
#endif
;

#endif
// EOF

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

#include "ccPars.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_REG_EXT
#else
#define CCPARS_REG_EXT extern
#endif

// Reg status enum for ccDebug only

CCPARS_GLOBAL_EXT struct ccpars_enum enum_reg_status[]
#ifdef GLOBALS
= {
    { REG_OK,                                "OK"                       },
    { REG_WARNING,                           "WARNING"                  },
    { REG_FAULT,                             "FAULT"                    },
    { 0,                                      NULL                      },
}
#endif
;

// Reg Jury's test result  enum for ccDebug only

CCPARS_GLOBAL_EXT struct ccpars_enum enum_reg_jurys_result[]
#ifdef GLOBALS
= {
    { REG_JR_OK,                             "OK"                       },
    { REG_JR_OHMS_PAR_TOO_SMALL,             "OHMS_PAR<1M"              },
    { REG_JR_PURE_DELAY_TOO_LARGE,           "Pure Delay>2.4 periods"   },
    { REG_JR_S0_IS_ZERO,                     "S[0]==0"                  },
    { REG_JR_SUM_S_IS_NEGATIVE,              "Sum(S)<0"                 },
    { REG_JR_SUM_EVEN_S_LESS_THAN_SUM_ODD_S, "Sum(Even S)<Sum(Odd S)"   },
    { REG_JR_S_HAS_UNSTABLE_POLE,            "S has unstable pole"      },
    { 0,                                      NULL                      },
}
#endif
;

// Reg RST source enum for ccDebug only

CCPARS_GLOBAL_EXT struct ccpars_enum enum_reg_rst_source[]
#ifdef GLOBALS
= {
    { REG_OPERATIONAL_RST_PARS,              "OPERATIONAL"              },
    { REG_TEST_RST_PARS,                     "TEST"                     },
    { 0,                                      NULL                      },
}
#endif
;

// Field and current Regulation parameters

struct ccpars_reg_pars
{
    uint32_t            period_iters       [REG_NUM_LOADS];     // Regulation period in iteration periods
    float               pure_delay_periods [REG_NUM_LOADS];     // Regulation pure delay in periods (0 to use automatic calculation)
    float               track_delay_periods[REG_NUM_LOADS];     // Regulation track delay in periods (0 to use automatic calculation)
    float               auxpole1_hz        [REG_NUM_LOADS];     // Frequency of (real) auxiliary pole 1
    float               auxpoles2_hz       [REG_NUM_LOADS];     // Frequency of (conjugate) auxiliary poles 2 & 3
    float               auxpoles2_z        [REG_NUM_LOADS];     // Damping of (conjugate) auxiliary poles 2 & 3
    float               auxpole4_hz        [REG_NUM_LOADS];     // Frequency of (real) auxiliary pole 4
    float               auxpole5_hz        [REG_NUM_LOADS];     // Frequency of (real) auxiliary pole 5
    struct reg_rst      rst;                                    // RST coefficients
    struct reg_rst      test_rst;                               // Test RST coefficients
};

CCPARS_REG_EXT struct ccpars_reg_pars ccpars_breg
#ifdef GLOBALS
= {//   Default value                         Parameter
        {   10,   10,   10,   10 },        // BREG PERIOD_ITERS
        {  0.0,  0.0,  0.0,  0.0 },        // BREG PURE_DELAY_PERIODS
        {  0.0,  0.0,  0.0,  0.0 },        // BREG TRACK_DELAY_PERIODS
        { 10.0, 10.0, 10.0, 10.0 },        // BREG AUXPOLE1_HZ
        { 10.0, 10.0, 10.0, 10.0 },        // BREG AUXPOLES2_HZ
        {  0.5,  0.5,  0.5,  0.5 },        // BREG AUXPOLES2_Z
        { 10.0, 10.0, 10.0, 10.0 },        // BREG AUXPOLE4_HZ
        { 10.0, 10.0, 10.0, 10.0 },        // BREG AUXPOLE5_HZ
        { {  0.0  },                       // BREG R
          {  0.0  },                       // BREG S
          {  0.0  } },                     // BREG T
        { {  0.0  },                       // BREG TEST_R
          {  0.0  },                       // BREG TEST_S
          {  0.0  } },                     // BREG TEST_T
}
#endif
;

CCPARS_REG_EXT struct ccpars_reg_pars ccpars_ireg
#ifdef GLOBALS
= {//   Default value                         Parameter
        {   10,   10,   10,   10 },        // IREG PERIOD_ITERS
        {  0.0,  0.0,  0.0,  0.0 },        // IREG PURE_DELAY_PERIODS
        {  0.0,  0.0,  0.0,  0.0 },        // IREG TRACK_DELAY_PERIODS
        { 10.0, 10.0, 10.0, 10.0 },        // IREG AUXPOLE1_HZ
        { 10.0, 10.0, 10.0, 10.0 },        // IREG AUXPOLES2_HZ
        {  0.5,  0.5,  0.5,  0.5 },        // IREG AUXPOLES2_Z
        { 10.0, 10.0, 10.0, 10.0 },        // IREG AUXPOLE4_HZ
        { 10.0, 10.0, 10.0, 10.0 },        // IREG AUXPOLE5_HZ
        { {  0.0  },                       // IREG R
          {  0.0  },                       // IREG S
          {  0.0  } },                     // IREG T
        { {  0.0  },                       // IREG TEST_R
          {  0.0  },                       // IREG TEST_S
          {  0.0  } },                     // IREG TEST_T
}
#endif
;

// Libreg structures

CCPARS_REG_EXT struct reg_conv conv;            // Libreg converter regulation structure

// Define Field and Current regulation parameters description structures

CCPARS_REG_EXT struct ccpars breg_pars[]
#ifdef GLOBALS
= {// "Signal name"          type,         max_n_els,         *enum,       *value,                             num_defaults, cyc_sel_step, flags
    { "PERIOD_ITERS",        PAR_UNSIGNED, REG_NUM_LOADS,      NULL, { .u = ccpars_breg.period_iters        }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "PURE_DELAY_PERIODS",  PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_breg.pure_delay_periods  }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "TRACK_DELAY_PERIODS", PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_breg.track_delay_periods }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "AUXPOLE1_HZ",         PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_breg.auxpole1_hz         }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "AUXPOLES2_HZ",        PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_breg.auxpoles2_hz        }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "AUXPOLES2_Z",         PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_breg.auxpoles2_z         }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "AUXPOLE4_HZ",         PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_breg.auxpole4_hz         }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "AUXPOLE5_HZ",         PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_breg.auxpole5_hz         }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "R",                   PAR_FLOAT,    REG_NUM_RST_COEFFS, NULL, { .f = ccpars_breg.rst.r               }, REG_NUM_RST_COEFFS, 0, PARS_FIXED_LENGTH },
    { "S",                   PAR_FLOAT,    REG_NUM_RST_COEFFS, NULL, { .f = ccpars_breg.rst.s               }, REG_NUM_RST_COEFFS, 0, PARS_FIXED_LENGTH },
    { "T",                   PAR_FLOAT,    REG_NUM_RST_COEFFS, NULL, { .f = ccpars_breg.rst.t               }, REG_NUM_RST_COEFFS, 0, PARS_FIXED_LENGTH },
    { "TEST_R",              PAR_FLOAT,    REG_NUM_RST_COEFFS, NULL, { .f = ccpars_breg.test_rst.r          }, REG_NUM_RST_COEFFS, 0, PARS_FIXED_LENGTH },
    { "TEST_S",              PAR_FLOAT,    REG_NUM_RST_COEFFS, NULL, { .f = ccpars_breg.test_rst.s          }, REG_NUM_RST_COEFFS, 0, PARS_FIXED_LENGTH },
    { "TEST_T",              PAR_FLOAT,    REG_NUM_RST_COEFFS, NULL, { .f = ccpars_breg.test_rst.t          }, REG_NUM_RST_COEFFS, 0, PARS_FIXED_LENGTH },
    { NULL }
}
#endif
;

CCPARS_REG_EXT struct ccpars ireg_pars[]
#ifdef GLOBALS
= {// "Signal name"          type,         max_n_els,         *enum,       *value,                             num_defaults, cyc_sel_step, flags
    { "PERIOD_ITERS",        PAR_UNSIGNED, REG_NUM_LOADS,      NULL, { .u = ccpars_ireg.period_iters        }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "PURE_DELAY_PERIODS",  PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_ireg.pure_delay_periods  }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "TRACK_DELAY_PERIODS", PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_ireg.track_delay_periods }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "AUXPOLE1_HZ",         PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_ireg.auxpole1_hz         }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "AUXPOLES2_HZ",        PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_ireg.auxpoles2_hz        }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "AUXPOLES2_Z",         PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_ireg.auxpoles2_z         }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "AUXPOLE4_HZ",         PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_ireg.auxpole4_hz         }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "AUXPOLE5_HZ",         PAR_FLOAT,    REG_NUM_LOADS,      NULL, { .f = ccpars_ireg.auxpole5_hz         }, REG_NUM_LOADS,      0, PARS_FIXED_LENGTH },
    { "R",                   PAR_FLOAT,    REG_NUM_RST_COEFFS, NULL, { .f = ccpars_ireg.rst.r               }, REG_NUM_RST_COEFFS, 0, PARS_FIXED_LENGTH },
    { "S",                   PAR_FLOAT,    REG_NUM_RST_COEFFS, NULL, { .f = ccpars_ireg.rst.s               }, REG_NUM_RST_COEFFS, 0, PARS_FIXED_LENGTH },
    { "T",                   PAR_FLOAT,    REG_NUM_RST_COEFFS, NULL, { .f = ccpars_ireg.rst.t               }, REG_NUM_RST_COEFFS, 0, PARS_FIXED_LENGTH },
    { "TEST_R",              PAR_FLOAT,    REG_NUM_RST_COEFFS, NULL, { .f = ccpars_ireg.test_rst.r          }, REG_NUM_RST_COEFFS, 0, PARS_FIXED_LENGTH },
    { "TEST_S",              PAR_FLOAT,    REG_NUM_RST_COEFFS, NULL, { .f = ccpars_ireg.test_rst.s          }, REG_NUM_RST_COEFFS, 0, PARS_FIXED_LENGTH },
    { "TEST_T",              PAR_FLOAT,    REG_NUM_RST_COEFFS, NULL, { .f = ccpars_ireg.test_rst.t          }, REG_NUM_RST_COEFFS, 0, PARS_FIXED_LENGTH },
    { NULL }
}
#endif
;

#endif
// EOF

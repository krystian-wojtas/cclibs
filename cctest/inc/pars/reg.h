/*---------------------------------------------------------------------------------------------------------*\
  File:     pars/reg.h                                                                  Copyright CERN 2011

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

  Purpose:  Structure for the regulation parameters file (-r load_file)

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_REG_H
#define CCPARS_REG_H

#include "ccpars.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_REG_EXT
#else
#define CCPARS_REG_EXT extern
#endif

// Regulation parameters structure

struct ccpars_reg
{
    // Regulation file parameters

    uint32_t            period_iters;              // Regulation period in iteration periods
    uint32_t            decimate;                  // Decimate measurement before regulation
    float               clbw;                      // Regulation closed loop bandwidth (real pole)
    float               clbw2;                     // Regulation closed loop bandwidth (conjugate poles)
    float               z;                         // Regulation conjugate poles damping factor (0.5-0.8)
    float               clbw3;                     // Regulation closed loop bandwidth (second real pole)
    float               clbw4;                     // Regulation closed loop bandwidth (third real pole)
    float               pure_delay;                // Regulation pure loop delay
    struct reg_rst      rst;                       // Regulation RST coefficient
    float               ol_time;                   // Open loop time
    float               ol_duration;               // Open loop duration

    // Regulation related variables

    uint32_t            status;                    // Reg parameter group status

    float               time;                      // Time of last iteration when regulation algorithm ran
    float               cl_time;                   // Close loop time = Open loop time + open loop duration
    float               feedforward_v_ref;         // Feedforward reference for START function
    unsigned            feedforward_control;       // Feedforward control for START function
};

CCPARS_REG_EXT struct ccpars_reg ccpars_reg;       // All fields are zero by default

CCPARS_REG_EXT struct reg_converter reg;           // Libreg converter regulation structure
CCPARS_REG_EXT struct reg_converter_pars reg_pars; // Libreg converter regulation parameters structure

// Regulation parameters description structure

CCPARS_REG_EXT struct ccpars reg_pars_list[]
#ifdef GLOBALS
= {// "Signal name"  TYPE,         max_vals,    min_vals, *enum, *value,                       num_defaults
    { "PERIOD_ITERS",PAR_UNSIGNED, 1,                1, NULL,  { .i = &ccpars_reg.period_iters    }, 0 },
    { "DECIMATE",    PAR_ENUM,     1,      0, enabled_disabled,{ .i = &ccpars_reg.decimate        }, 1 },
    { "TRACK_DELAY", PAR_FLOAT,    1,                0, NULL,  { .f = &ccpars_reg.rst.track_delay }, 1 },
    { "CLBW",        PAR_FLOAT,    1,                0, NULL,  { .f = &ccpars_reg.clbw            }, 1 },
    { "CLBW2",       PAR_FLOAT,    1,                0, NULL,  { .f = &ccpars_reg.clbw2           }, 1 },
    { "Z",           PAR_FLOAT,    1,                0, NULL,  { .f = &ccpars_reg.z               }, 1 },
    { "CLBW3",       PAR_FLOAT,    1,                0, NULL,  { .f = &ccpars_reg.clbw3           }, 1 },
    { "CLBW4",       PAR_FLOAT,    1,                0, NULL,  { .f = &ccpars_reg.clbw4           }, 1 },
    { "PURE_DELAY",  PAR_FLOAT,    1,                0, NULL,  { .f = &ccpars_reg.pure_delay      }, 1 },
    { "R",           PAR_FLOAT,    REG_N_RST_COEFFS, 0, NULL,  { .f =  ccpars_reg.rst.r           }, 0 },
    { "S",           PAR_FLOAT,    REG_N_RST_COEFFS, 0, NULL,  { .f =  ccpars_reg.rst.s           }, 0 },
    { "T",           PAR_FLOAT,    REG_N_RST_COEFFS, 0, NULL,  { .f =  ccpars_reg.rst.t           }, 0 },
    { "OL_TIME",     PAR_FLOAT,    1,                0, NULL,  { .f = &ccpars_reg.ol_time         }, 1 },
    { "OL_DURATION", PAR_FLOAT,    1,                0, NULL,  { .f = &ccpars_reg.ol_duration     }, 1 },
    { NULL }
}
#endif
;

#endif
/*---------------------------------------------------------------------------------------------------------*\
  End of file: pars/reg.h
\*---------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/pars/pc.h                                                        Copyright CERN 2014

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

  Purpose:  Structure for the power converter mnodel parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_PC_H
#define CCPARS_PC_H

#include "ccCmds.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_PC_EXT
#else
#define CCPARS_PC_EXT extern
#endif

// Regulation actuation enum constants come from libreg

CCPARS_GLOBAL_EXT struct ccpars_enum enum_reg_actuation[]
#ifdef GLOBALS
= {
    { REG_VOLTAGE_REF,    "VOLTAGE" },
    { REG_CURRENT_REF,    "CURRENT" },
    { 0,                   NULL     },
}
#endif
;

// Power converter parameters structure

struct ccpars_pc
{
    enum reg_actuation          actuation;          // Power converter actuation (VOLTAGE REF or CURRENT REF)
    float                       act_delay_iters;    // Power converter control delay in iterations
    float                       quantization;       // Actuation quantization (V or A)
    float                       bandwidth;          // Power converter (voltage source or current source) second order bandwidth
    float                       z;                  // Second order damping factor
    float                       tau_zero;           // Second order time constant of zero (0 if not required)
    struct reg_sim_pc_pars      sim_pc_pars;        // Power converter third order model if bandwidth is zero
};

CCPARS_PC_EXT struct ccpars_pc ccpars_pc
#ifdef GLOBALS
= {//   Default value               Parameter
        REG_VOLTAGE_REF,         // PC ACTUATION
        1.0,                     // PC ACT_DELAY_ITERS
        0.0,                     // PC QUANTIZATION
        200.0,                   // PC BANDWIDTH
        0.9,                     // PC Z
        0.0,                     // PC TAU_ZERO
        {  { 1.0 },              // PC SIM_NUM
           { 1.0 }  },           // PC SIM_DEN
}
#endif
;

// Power converter parameters description structure

CCPARS_PC_EXT struct ccpars pc_pars[]
#ifdef GLOBALS
= {// "Signal name"      type,      max_n_els,            *enum,                      *value,                       num_defaults,    cyc_sel_step, flags
    { "ACTUATION",       PAR_ENUM,  1,                     enum_reg_actuation, { .u = &ccpars_pc.actuation       }, 1,                     0, 0                 },
    { "ACT_DELAY_ITERS", PAR_FLOAT, 1,                     NULL,               { .f = &ccpars_pc.act_delay_iters }, 1,                     0, 0                 },
    { "QUANTIZATION",    PAR_FLOAT, 1,                     NULL,               { .f = &ccpars_pc.quantization    }, 1,                     0, 0                 },
    { "BANDWIDTH",       PAR_FLOAT, 1,                     NULL,               { .f = &ccpars_pc.bandwidth       }, 1,                     0, 0                 },
    { "Z",               PAR_FLOAT, 1,                     NULL,               { .f = &ccpars_pc.z               }, 1,                     0, 0                 },
    { "TAU_ZERO",        PAR_FLOAT, 1,                     NULL,               { .f = &ccpars_pc.tau_zero        }, 1,                     0, 0                 },
    { "SIM_NUM",         PAR_FLOAT, REG_NUM_PC_SIM_COEFFS, NULL,               { .f =  ccpars_pc.sim_pc_pars.num }, REG_NUM_PC_SIM_COEFFS, 0, PARS_FIXED_LENGTH },
    { "SIM_DEN",         PAR_FLOAT, REG_NUM_PC_SIM_COEFFS, NULL,               { .f =  ccpars_pc.sim_pc_pars.den }, REG_NUM_PC_SIM_COEFFS, 0, PARS_FIXED_LENGTH },
    { NULL }
}
#endif
;

#endif
// EOF


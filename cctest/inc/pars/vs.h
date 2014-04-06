/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/pars/vs.h                                                        Copyright CERN 2014

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

  Purpose:  Structure for the voltage source parameters group

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_VS_H
#define CCPARS_VS_H

#include "ccpars.h"
#include "libreg.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_VS_EXT
#else
#define CCPARS_VS_EXT extern
#endif

// Voltage source parameters structure

struct ccpars_vs
{
    // Voltage source file parameter

    float                       v_ref_delay_iters;  // Voltage source control delay in iterations
    float                       bandwidth;          // Voltage source bandwidth
    float                       z;                  // Damping factor
    float                       tau_zero;           // Time constant of zero
    struct reg_sim_vs_pars      sim_vs_pars;        // Voltage source simulation model if bandwidth is zero

    // Voltage source related variables

    uint32_t                    trip_flag;          // Voltage source tripped by measurement limit
};

CCPARS_VS_EXT struct ccpars_vs ccpars_vs
#ifdef GLOBALS
= {//   Default value               Parameter
        1.0,                     // VS.V_REF_DELAY_ITERS
        0.0,                     // VS.BANDWIDTH
        0.9,                     // VS.Z
        0.0,                     // VS.TAU_ZERO
        {  { 1.0 },              // VS.SIM_NUM
           { 1.0 }  },           // VS.SIM_DEN
}
#endif
;

// Voltage source parameters description structure

CCPARS_VS_EXT struct ccpars vs_pars_list[]
#ifdef GLOBALS
= {// "Signal name"       TYPE,   max_vals,        min_vals,*enum,         *value,                     num_defaults
    { "V_REF_DELAY_ITERS",PAR_FLOAT, 1,                   0, NULL,  { .f = &ccpars_vs.v_ref_delay_iters }, 1 },
    { "BANDWIDTH",        PAR_FLOAT, 1,                   0, NULL,  { .f = &ccpars_vs.bandwidth         }, 1 },
    { "Z",                PAR_FLOAT, 1,                   0, NULL,  { .f = &ccpars_vs.z                 }, 1 },
    { "TAU_ZERO",         PAR_FLOAT, 1,                   0, NULL,  { .f = &ccpars_vs.tau_zero          }, 1 },
    { "SIM_NUM",          PAR_FLOAT, REG_N_VS_SIM_COEFFS, 0, NULL,  { .f =  ccpars_vs.sim_vs_pars.num   }, 1 },
    { "SIM_DEN",          PAR_FLOAT, REG_N_VS_SIM_COEFFS, 0, NULL,  { .f =  ccpars_vs.sim_vs_pars.den   }, 1 },
    { NULL }
}
#endif
;

#endif
// EOF


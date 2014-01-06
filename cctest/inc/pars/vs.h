/*---------------------------------------------------------------------------------------------------------*\
  File:     pars/vs.h                                                                   Copyright CERN 2011

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

  Purpose:  Structure for the voltage source parameters file (-s vs_file)

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

    float                       v_ref_delay;      // Voltage source control delay
    float                       v_meas_delay;     // Voltage measurement delay
    struct reg_meas_pars        v_meas_pars;      // Voltage measurement IIR filter parameters
    float                       track_delay;      // Voltage source track delay
    float                       bandwidth;        // Voltage source bandwidth
    float                       z;                // Damping factor
    float                       tau_zero;         // Time constant of zero
    float                       v_sim_noise;      // Simulated voltage measurement noise level
    struct reg_sim_vs_pars      sim_vs_pars;      // Voltage source simulation model

    // Vvoltage source related variables

    uint32_t                    status;           // Voltage source parameter group status
    uint32_t                    trip_flag;        // Voltage source tripped by measurement limit
};

CCPARS_VS_EXT struct ccpars_vs ccpars_vs
#ifdef GLOBALS
= {
    1.0,                                        // V_REF_DELAY
    0.0,                                        // V_MEAS_DELAY
    {  { 1.0 }, { 1.0 }  },                     // V_MEAS_NUM, V_MEAS_DEN: Default VS measurement IIR filter
    1.0,                                        // TRACK_DELAY
    0.0,                                        // NATURAL_FREQ
    0.5,                                        // Z
    0.0,                                        // TAU_ZERO
    0.0,                                        // V_SIM_NOISE
    {  { 1.0 }, { 1.0 }  },                     // SIM_NUM, SIM_DEN: Default VS response is no delay
}
#endif
;

// Voltage source parameters description structure

CCPARS_VS_EXT struct ccpars vs_pars_list[]
#ifdef GLOBALS
= {// "Signal name"    TYPE,   max_vals,        min_vals,*enum,     *value,                     num_defaults
    { "V_REF_DELAY",   PAR_FLOAT, 1,                   0, NULL,  { .f = &ccpars_vs.v_ref_delay      }, 1 },
    { "V_MEAS_DELAY",  PAR_FLOAT, 1,                   0, NULL,  { .f = &ccpars_vs.v_meas_delay     }, 1 },
    { "V_MEAS_NUM",    PAR_FLOAT, REG_N_IIR_COEFFS,    0, NULL,  { .f =  ccpars_vs.v_meas_pars.num  }, 1 },
    { "V_MEAS_DEN",    PAR_FLOAT, REG_N_IIR_COEFFS,    0, NULL,  { .f =  ccpars_vs.v_meas_pars.den  }, 1 },
    { "TRACK_DELAY",   PAR_FLOAT, 1,                   0, NULL,  { .f = &ccpars_vs.track_delay      }, 1 },
    { "BANDWIDTH",     PAR_FLOAT, 1,                   0, NULL,  { .f = &ccpars_vs.bandwidth        }, 1 },
    { "Z",             PAR_FLOAT, 1,                   0, NULL,  { .f = &ccpars_vs.z                }, 1 },
    { "TAU_ZERO",      PAR_FLOAT, 1,                   0, NULL,  { .f = &ccpars_vs.tau_zero         }, 1 },
    { "V_SIM_NOISE",   PAR_FLOAT, 1,                   0, NULL,  { .f = &ccpars_vs.v_sim_noise      }, 1 },
    { "SIM_NUM",       PAR_FLOAT, REG_N_VS_SIM_COEFFS, 0, NULL,  { .f =  ccpars_vs.sim_vs_pars.num  }, 1 },
    { "SIM_DEN",       PAR_FLOAT, REG_N_VS_SIM_COEFFS, 0, NULL,  { .f =  ccpars_vs.sim_vs_pars.den  }, 1 },
    { NULL }
}
#endif
;

#endif
// EOF


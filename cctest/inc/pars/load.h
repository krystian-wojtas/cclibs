/*---------------------------------------------------------------------------------------------------------*\
  File:     pars/load.h                                                                 Copyright CERN 2011

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

  Purpose:  Structure for the load parameters file (-l load_file)

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_LOAD_H
#define CCPARS_LOAD_H

#include "ccpars.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_LOAD_EXT
#else
#define CCPARS_LOAD_EXT extern
#endif

// Load parameters structure

struct ccpars_load
{
    // Load file parameters

    float                   ohms_ser;         // Series resistance
    float                   ohms_par;         // Parallel resistance
    float                   ohms_mag;         // Magnet resistance

    float                   henrys;           // Unsaturated magnet inductance
    float                   henrys_sat;       // Saturated magnet inductance

    float                   i_sat_start;      // Current at start of saturation
    float                   i_sat_end;        // Current at end of saturation

    float                   gauss_per_amp;    // Field to current ratio (G/A)

    float                   i_meas_delay;     // Current measurement delay
    struct reg_meas_pars    i_meas_pars;      // Current measurement IIR filter parameters

    float                   b_meas_delay;     // Field measurement delay
    struct reg_meas_pars    b_meas_pars;      // Field measurement IIR filter parameters

    float                   perturb_volts;    // Open loop voltage perturbation
    float                   perturb_time;     // Time for open loop voltage perturbation

    float                   i_sim_noise;      // Current measurement noise level
    float                   b_sim_noise;      // Field measurement noise level
    float                   sim_tc_error;     // Error factor for simulation

    // Load related variables

    uint32_t    status;                         // Load parameter group status

    float       sim_i_load;                     // Simulated load current
};

CCPARS_LOAD_EXT struct ccpars_load ccpars_load
#ifdef GLOBALS
= {
    1.0,                            // OHMS_SER
    1.0E9,                          // OHMS_PAR
    1.0,                            // OHMS_MAG
    1.0,                            // HENRYS
    1.0,                            // HENRYS_SAT
    0.0,                            // I_SAT_START
    0.0,                            // I_SAT_END
    1.0,                            // GAUSS_PER_AMP
    0.0,                            // I_MEAS_DELAY
    {  { 1.0 }, { 1.0 }  },         // I_MEAS_NUM, I_MEAS_DEN: Default current measurement IIR filter
    0.0,                            // B_MEAS_DELAY
    {  { 1.0 }, { 1.0 }  },         // B_MEAS_NUM, B_MEAS_DEN: Default field measurement IIR filter
    0.0,                            // PERTURB_VOLTS
    0.0,                            // PERTURB_TIME
    0.0,                            // I_SIM_NOISE
    0.0,                            // B_SIM_NOISE
    0.0,                            // SIM_TC_ERROR
}
#endif
;

// Load parameters description structure

CCPARS_LOAD_EXT struct ccpars load_pars_list[]
#ifdef GLOBALS
= {// "Signal name"      TYPE,max_vals,min_vals,*enum,  *value,                       num_defaults
    { "OHMS_SER",        PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.ohms_ser        }, 1 },
    { "OHMS_PAR",        PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.ohms_par        }, 1 },
    { "OHMS_MAG",        PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.ohms_mag        }, 1 },
    { "HENRYS",          PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.henrys          }, 1 },
    { "HENRYS_SAT",      PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.henrys_sat      }, 1 },
    { "I_SAT_START",     PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.i_sat_start     }, 1 },
    { "I_SAT_END",       PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.i_sat_end       }, 1 },
    { "GAUSS_PER_AMP",   PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.gauss_per_amp   }, 1 },
    { "I_MEAS_DELAY",    PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.i_meas_delay    }, 1 },
    { "I_MEAS_NUM",      PAR_FLOAT, REG_N_IIR_COEFFS, 0, NULL, { .f =  ccpars_load.i_meas_pars.num }, 1 },
    { "I_MEAS_DEN",      PAR_FLOAT, REG_N_IIR_COEFFS, 0, NULL, { .f =  ccpars_load.i_meas_pars.den }, 1 },
    { "B_MEAS_DELAY",    PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.b_meas_delay    }, 1 },
    { "B_MEAS_NUM",      PAR_FLOAT, REG_N_IIR_COEFFS, 0, NULL, { .f =  ccpars_load.b_meas_pars.num }, 1 },
    { "B_MEAS_DEN",      PAR_FLOAT, REG_N_IIR_COEFFS, 0, NULL, { .f =  ccpars_load.b_meas_pars.den }, 1 },
    { "PERTURB_VOLTS",   PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.perturb_volts   }, 1 },
    { "PERTURB_TIME",    PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.perturb_time    }, 1 },
    { "I_SIM_NOISE",     PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.i_sim_noise     }, 1 },
    { "B_SIM_NOISE",     PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.b_sim_noise     }, 1 },
    { "SIM_TC_ERROR",    PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_load.sim_tc_error    }, 1 },
    { NULL }
}
#endif
;

#endif
// EOF


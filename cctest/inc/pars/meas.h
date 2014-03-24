/*---------------------------------------------------------------------------------------------------------*\
  File:     pars/meas.h                                                                 Copyright CERN 2014

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

  Purpose:  Structure for the meas parameters group

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_MEAS_H
#define CCPARS_MEAS_H

#include "ccpars.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_MEAS_EXT
#else
#define CCPARS_MEAS_EXT extern
#endif

// Meas parameters structure

struct ccpars_meas
{
    // Meas parameters

    float                   v_meas_delay;     // Voltage measurement delay
    float                   i_meas_delay;     // Current measurement delay
    float                   b_meas_delay;     // Field measurement delay

    struct reg_meas_pars    i_meas_pars;      // Current measurement IIR filter parameters
    struct reg_meas_pars    b_meas_pars;      // Field measurement IIR filter parameters

    float                   v_sim_noise_pp;   // Simulated voltage measurement noise level
    float                   i_sim_noise_pp;   // Current measurement noise level
    float                   b_sim_noise_pp;   // Field measurement noise level
};

CCPARS_MEAS_EXT struct ccpars_meas ccpars_meas
#ifdef GLOBALS
= {
    0.0,                            // V_MEAS_DELAY
    0.0,                            // I_MEAS_DELAY
    0.0,                            // B_MEAS_DELAY
    {  { 1.0 }, { 1.0 }  },         // I_MEAS_NUM, I_MEAS_DEN: Default current measurement IIR filter
    {  { 1.0 }, { 1.0 }  },         // B_MEAS_NUM, B_MEAS_DEN: Default field measurement IIR filter
    0.0,                            // I_SIM_NOISE_PP
    0.0,                            // I_SIM_NOISE_PP
    0.0,                            // B_SIM_NOISE_PP
}
#endif
;

// Meas parameters description structure

CCPARS_MEAS_EXT struct ccpars meas_pars_list[]
#ifdef GLOBALS
= {// "Signal name"      TYPE,max_vals,min_vals,*enum,  *value,                       num_defaults
    { "V_MEAS_DELAY",    PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_meas.v_meas_delay    }, 1 },
    { "I_MEAS_DELAY",    PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_meas.i_meas_delay    }, 1 },
    { "B_MEAS_DELAY",    PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_meas.b_meas_delay    }, 1 },
    { "I_MEAS_NUM",      PAR_FLOAT, REG_N_IIR_COEFFS, 0, NULL, { .f =  ccpars_meas.i_meas_pars.num }, 1 },
    { "I_MEAS_DEN",      PAR_FLOAT, REG_N_IIR_COEFFS, 0, NULL, { .f =  ccpars_meas.i_meas_pars.den }, 1 },
    { "B_MEAS_NUM",      PAR_FLOAT, REG_N_IIR_COEFFS, 0, NULL, { .f =  ccpars_meas.b_meas_pars.num }, 1 },
    { "B_MEAS_DEN",      PAR_FLOAT, REG_N_IIR_COEFFS, 0, NULL, { .f =  ccpars_meas.b_meas_pars.den }, 1 },
    { "V_SIM_NOISE_PP",  PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_meas.v_sim_noise_pp  }, 1 },
    { "I_SIM_NOISE_PP",  PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_meas.i_sim_noise_pp  }, 1 },
    { "B_SIM_NOISE_PP",  PAR_FLOAT,  1,               0, NULL, { .f = &ccpars_meas.b_sim_noise_pp  }, 1 },
    { NULL }
}
#endif
;

#endif
// EOF


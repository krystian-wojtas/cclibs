/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/pars/load.h                                                      Copyright CERN 2014

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

  Purpose:  Structure for the load parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_LOAD_H
#define CCPARS_LOAD_H

#include "ccPars.h"

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

    float       ohms_ser      [REG_NUM_LOADS];      // Series resistance
    float       ohms_par      [REG_NUM_LOADS];      // Parallel resistance
    float       ohms_mag      [REG_NUM_LOADS];      // Magnet resistance
    float       henrys        [REG_NUM_LOADS];      // Unsaturated magnet inductance
    float       henrys_sat    [REG_NUM_LOADS];      // Saturated magnet inductance
    float       i_sat_start   [REG_NUM_LOADS];      // Current at start of saturation
    float       i_sat_end     [REG_NUM_LOADS];      // Current at end of saturation
    float       gauss_per_amp [REG_NUM_LOADS];      // Field to current ratio (G/A)

    uint32_t    select;                             // Operational load selector
    uint32_t    test_select;                        // Test load selector
    float       sim_tc_error;                       // Error factor for simulation

    float       perturb_volts;                      // Open loop voltage perturbation
    float       perturb_time;                       // Time for open loop voltage perturbation

    enum reg_enabled_disabled pol_swi_auto;         // Auto polarity switch will follow function
};

CCPARS_LOAD_EXT struct ccpars_load ccpars_load
#ifdef GLOBALS
= {//   Default values                        Parameter
    {   0.5,   0.5,   0.5,   0.5, },       // LOAD OHMS_SER
    { 1.0E9, 1.0E9, 1.0E9, 1.0E9, },       // LOAD OHMS_PAR
    {   1.0,   1.0,   1.0,   1.0, },       // LOAD OHMS_MAG
    {   1.0,   1.0,   1.0,   1.0, },       // LOAD HENRYS
    {   1.0,   1.0,   1.0,   1.0, },       // LOAD HENRYS_SAT
    {   0.0,   0.0,   0.0,   0.0, },       // LOAD I_SAT_START
    {   0.0,   0.0,   0.0,   0.0, },       // LOAD I_SAT_END
    {   1.2,   1.2,   1.2,   1.2, },       // LOAD GAUSS_PER_AMP
          0,                               // LOAD SELECT
          0,                               // LOAD TEST_SELECT
        0.0,                               // LOAD SIM_TC_ERROR
        0.0,                               // LOAD PERTURB_VOLTS
        0.0,                               // LOAD PERTURB_TIME
        REG_DISABLED,                      // LOAD POL_SWI_AUTO
}
#endif
;

// Load parameters description structure

CCPARS_LOAD_EXT struct ccpars load_pars[]
#ifdef GLOBALS
= {// "Signal name"    type,         max_n_els,  *enum,               *value,                      num_defaults,cyc_sel_step,flags
    { "OHMS_SER",      PAR_FLOAT,    REG_NUM_LOADS, NULL,        { .f =  ccpars_load.ohms_ser      }, REG_NUM_LOADS, 0, PARS_FIXED_LENGTH },
    { "OHMS_PAR",      PAR_FLOAT,    REG_NUM_LOADS, NULL,        { .f =  ccpars_load.ohms_par      }, REG_NUM_LOADS, 0, PARS_FIXED_LENGTH },
    { "OHMS_MAG",      PAR_FLOAT,    REG_NUM_LOADS, NULL,        { .f =  ccpars_load.ohms_mag      }, REG_NUM_LOADS, 0, PARS_FIXED_LENGTH },
    { "HENRYS",        PAR_FLOAT,    REG_NUM_LOADS, NULL,        { .f =  ccpars_load.henrys        }, REG_NUM_LOADS, 0, PARS_FIXED_LENGTH },
    { "HENRYS_SAT",    PAR_FLOAT,    REG_NUM_LOADS, NULL,        { .f =  ccpars_load.henrys_sat    }, REG_NUM_LOADS, 0, PARS_FIXED_LENGTH },
    { "I_SAT_START",   PAR_FLOAT,    REG_NUM_LOADS, NULL,        { .f =  ccpars_load.i_sat_start   }, REG_NUM_LOADS, 0, PARS_FIXED_LENGTH },
    { "I_SAT_END",     PAR_FLOAT,    REG_NUM_LOADS, NULL,        { .f =  ccpars_load.i_sat_end     }, REG_NUM_LOADS, 0, PARS_FIXED_LENGTH },
    { "GAUSS_PER_AMP", PAR_FLOAT,    REG_NUM_LOADS, NULL,        { .f =  ccpars_load.gauss_per_amp }, REG_NUM_LOADS, 0, PARS_FIXED_LENGTH },
    { "SELECT",        PAR_UNSIGNED, 1,             NULL,        { .u = &ccpars_load.select        }, 1,             0, 0                 },
    { "TEST_SELECT",   PAR_UNSIGNED, 1,             NULL,        { .u = &ccpars_load.test_select   }, 1,             0, 0                 },
    { "SIM_TC_ERROR",  PAR_FLOAT,    1,             NULL,        { .f = &ccpars_load.sim_tc_error  }, 1,             0, 0                 },
    { "PERTURB_VOLTS", PAR_FLOAT,    1,             NULL,        { .f = &ccpars_load.perturb_volts }, 1,             0, 0                 },
    { "PERTURB_TIME",  PAR_FLOAT,    1,             NULL,        { .f = &ccpars_load.perturb_time  }, 1,             0, 0                 },
    { "POL_SWI_AUTO",  PAR_ENUM,     1, enum_enabled_disabled,   { .u = &ccpars_load.pol_swi_auto  }, 1,             0, 0                 },
    { NULL }
}
#endif
;

#endif
// EOF


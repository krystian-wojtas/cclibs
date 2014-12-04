/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/pars/limits.h                                                    Copyright CERN 2014

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

  Purpose:  Structure for the limit parameters file

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_LIMITS_H
#define CCPARS_LIMITS_H

#include "ccPars.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_LIMITS_EXT
#else
#define CCPARS_LIMITS_EXT extern
#endif

// Limit parameters structure

struct ccpars_limits
{
    float   b_pos              [REG_N_LOADS];      // Field positive trip limit
    float   b_min              [REG_N_LOADS];      // Field minimum regulation limit
    float   b_neg              [REG_N_LOADS];      // Field negative trip limit
    float   b_rate             [REG_N_LOADS];      // Field rate of change clip limit
    float   b_acceleration     [REG_N_LOADS];      // Field acceleration clip limit
    float   b_closeloop        [REG_N_LOADS];      // Field regulation closed loop limit
    float   b_low              [REG_N_LOADS];      // Field low limit
    float   b_zero             [REG_N_LOADS];      // Field zero limit
    float   b_err_warning      [REG_N_LOADS];      // Field regulation error warning limit
    float   b_err_fault        [REG_N_LOADS];      // Field regulation error fault limit

    float   i_pos              [REG_N_LOADS];      // Current positive trip limit
    float   i_min              [REG_N_LOADS];      // Current minimum regulation limit
    float   i_neg              [REG_N_LOADS];      // Current negative trip limit
    float   i_rate             [REG_N_LOADS];      // Current rate of change clip limit
    float   i_acceleration     [REG_N_LOADS];      // Current acceleration clip limit
    float   i_closeloop        [REG_N_LOADS];      // Current regulation closed loop limit
    float   i_low              [REG_N_LOADS];      // Current low limit
    float   i_zero             [REG_N_LOADS];      // Current zero limit
    float   i_err_warning      [REG_N_LOADS];      // Current regulation error warning limit
    float   i_err_fault        [REG_N_LOADS];      // Current regulation error fault limit
    float   i_quadrants41      [2];                // Current range for voltage exclusion zone for quadrants 4 and 1

    float   i_rms_tc;                              // Converter RMS current filter time constant
    float   i_rms_warning;                         // Converter RMS current warning limit
    float   i_rms_fault;                           // Converter RMS current fault limit

    float   i_rms_load_tc      [REG_N_LOADS];      // Load RMS current filter time constant
    float   i_rms_load_warning [REG_N_LOADS];      // Load RMS current warning limit
    float   i_rms_load_fault   [REG_N_LOADS];      // Load RMS current fault limit

    float   v_pos              [REG_N_LOADS];      // Voltage positive clip limit
    float   v_neg              [REG_N_LOADS];      // Voltage negative clip limit
    float   v_rate;                                // Voltage rate of change clip limit
    float   v_acceleration;                        // Voltage acceleration clip limit
    float   v_err_warning;                         // Voltage regulation error warning limit
    float   v_err_fault;                           // Voltage regulation error fault limit
    float   v_quadrants41      [2];                // Voltage exclusion zone limits for quadrants 4 and 1

    enum reg_enabled_disabled  invert;             // Invert real-time limits (true if polarity switch is negative)
};

CCPARS_LIMITS_EXT struct ccpars_limits ccpars_limits
#ifdef GLOBALS
= {// Default values                          Parameter
    {   10.0,   10.0,   10.0,   10.0 },  // LIMITS B_POS
    {    1.0,    1.0,    1.0,    1.0 },  // LIMITS B_MIN
    {  -10.0,  -10.0,  -10.0,  -10.0 },  // LIMITS B_NEG
    {    5.0,    5.0,    5.0,    5.0 },  // LIMITS B_RATE
    {  1.0E6,  1.0E6,  1.0E6,  1.0E6 },  // LIMITS B_ACCELERATION
    {    0.5,    0.5,    0.5,    0.5 },  // LIMITS B_CLOSELOOP
    {    0.0,    0.0,    0.0,    0.0 },  // LIMITS B_LOW
    {    0.0,    0.0,    0.0,    0.0 },  // LIMITS B_ZERO
    {    0.0,    0.0,    0.0,    0.0 },  // LIMITS B_ERR_WARNING
    {    0.0,    0.0,    0.0,    0.0 },  // LIMITS B_ERR_FAULT
    {   10.0,   10.0,   10.0,   10.0 },  // LIMITS I_POS
    {    1.0,    1.0,    1.0,    1.0 },  // LIMITS I_MIN
    {  -10.0,  -10.0,  -10.0,  -10.0 },  // LIMITS I_NEG
    {    5.0,    5.0,    5.0,    5.0 },  // LIMITS I_RATE
    {  1.0E6,  1.0E6,  1.0E6,  1.0E6 },  // LIMITS I_ACCELERATION
    {    0.5,    0.5,    0.5,    0.5 },  // LIMITS I_CLOSELOOP
    {    0.0,    0.0,    0.0,    0.0 },  // LIMITS I_LOW
    {    0.0,    0.0,    0.0,    0.0 },  // LIMITS I_ZERO
    {    0.0,    0.0,    0.0,    0.0 },  // LIMITS I_ERR_WARNING
    {    0.0,    0.0,    0.0,    0.0 },  // LIMITS I_ERR_FAULT
    {    0.0,    0.0                 },  // LIMITS I_QUADRANTS41
         0.0                          ,  // LIMITS I_RMS_TC
         0.0                          ,  // LIMITS I_RMS_WARNING
         0.0                          ,  // LIMITS I_RMS_FAULT
    {    0.0,    0.0,    0.0,    0.0 },  // LIMITS I_RMS_LOAD_TC
    {    0.0,    0.0,    0.0,    0.0 },  // LIMITS I_RMS_LOAD_WARNING
    {    0.0,    0.0,    0.0,    0.0 },  // LIMITS I_RMS_LOAD_FAULT
    {  100.0,  100.0,  100.0,  100.0 },  // LIMITS V_POS
    { -100.0, -100.0, -100.0, -100.0 },  // LIMITS V_NEG
       1.0E3                          ,  // LIMITS V_RATE
       1.0E6                          ,  // LIMITS V_ACCELERATION
         0.0                          ,  // LIMITS V_ERR_WARNING
         0.0                          ,  // LIMITS V_ERR_FAULT
    {    0.0,   0.0                  },  // LIMITS V_QUADRANTS41
         REG_DISABLED                 ,  // LIMITS INVERT
}
#endif
;

// Global parameters description structure

CCPARS_LIMITS_EXT struct ccpars limits_pars[]
#ifdef GLOBALS
= {// "Signal name"         type,      max_n_els,  *enum,               *value,                             num_defaults, flags
    { "B_POS",              PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.b_pos              }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "B_MIN",              PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.b_min              }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "B_NEG",              PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.b_neg              }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "B_RATE",             PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.b_rate             }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "B_ACCELERATION",     PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.b_acceleration     }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "B_CLOSELOOP",        PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.b_closeloop        }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "B_LOW",              PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.b_low              }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "B_ZERO",             PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.b_zero             }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "B_ERR_WARNING",      PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.b_err_warning      }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "B_ERR_FAULT",        PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.b_err_fault        }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_POS",              PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_pos              }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_MIN",              PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_min              }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_NEG",              PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_neg              }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_RATE",             PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_rate             }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_ACCELERATION",     PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_acceleration     }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_CLOSELOOP",        PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_closeloop        }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_LOW",              PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_low              }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_ZERO",             PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_zero             }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_ERR_WARNING",      PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_err_warning      }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_ERR_FAULT",        PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_err_fault        }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_QUADRANTS41",      PAR_FLOAT, 2,           NULL,        { .f =  ccpars_limits.i_quadrants41      }, 2,           PARS_FIXED_LENGTH },
    { "I_RMS_TC",           PAR_FLOAT, 1,           NULL,        { .f = &ccpars_limits.i_rms_tc           }, 1,           0                 },
    { "I_RMS_WARNING",      PAR_FLOAT, 1,           NULL,        { .f = &ccpars_limits.i_rms_warning      }, 1,           0                 },
    { "I_RMS_FAULT",        PAR_FLOAT, 1,           NULL,        { .f = &ccpars_limits.i_rms_fault        }, 1,           0                 },
    { "I_RMS_LOAD_TC",      PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_rms_load_tc      }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_RMS_LOAD_WARNING", PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_rms_load_warning }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "I_RMS_LOAD_FAULT",   PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.i_rms_load_fault   }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "V_POS",              PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.v_pos              }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "V_NEG",              PAR_FLOAT, REG_N_LOADS, NULL,        { .f =  ccpars_limits.v_neg              }, REG_N_LOADS, PARS_FIXED_LENGTH },
    { "V_RATE",             PAR_FLOAT, 1,           NULL,        { .f = &ccpars_limits.v_rate             }, 1,           0                 },
    { "V_ACCELERATION",     PAR_FLOAT, 1,           NULL,        { .f = &ccpars_limits.v_acceleration     }, 1,           0                 },
    { "V_ERR_WARNING",      PAR_FLOAT, 1,           NULL,        { .f = &ccpars_limits.v_err_warning      }, 1,           0                 },
    { "V_ERR_FAULT",        PAR_FLOAT, 1,           NULL,        { .f = &ccpars_limits.v_err_fault        }, 1,           0                 },
    { "V_QUADRANTS41",      PAR_FLOAT, 2,           NULL,        { .f =  ccpars_limits.v_quadrants41      }, 2,           PARS_FIXED_LENGTH },
    { "INVERT",             PAR_ENUM,  1, enum_enabled_disabled, { .u = &ccpars_limits.invert             }, 1,           0                 },
    { NULL }
}
#endif
;

#endif
// EOF

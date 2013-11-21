/*---------------------------------------------------------------------------------------------------------*\
  File:     ccsigs.h                                                                     Copyright CERN 2011

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

  Purpose:  Header file for FG library test program output signals

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef FGSIGS_H
#define FGSIGS_H

#include <stdint.h>

#include "ccpars.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define FGSIGS_EXT
#else
#define FGSIGS_EXT extern
#endif

// Signal constants

enum ccsig_type
{
    ANALOG,
    DIGITAL,
    CURSOR
};

enum ccsig_idx
{
    CSR_LOAD,                   // Load events
    CSR_REGMODE,                // Regulation mode events
    CSR_REF,                    // Reference events
    ANA_B_REF,                  // Field reference
    ANA_B_REF_LIMITED,          // Field reference after limits applied
    ANA_B_REF_RST,              // Field reference stored in RST history (limited and after back-calculation)
    ANA_B_MEAS,                 // Unfiltered field measurement
    ANA_B_MEAS_FLTR,            // Filtered field measurement
    ANA_B_REG,                  // Field measurement used for regulation (filtered and/or decimated)
    ANA_B_ERR,                  // Field regulation error
    ANA_I_REF,                  // Current reference
    ANA_I_REF_LIMITED,          // Current reference after limits applied
    ANA_I_REF_RST,              // Current reference stored in RST history (limited and after back-calculation)
    ANA_I_MEAS,                 // Unfiltered measured current
    ANA_I_MEAS_FLTR,            // Filtered measured current
    ANA_I_REG,                  // Measured current used for regulation (filtered and/or decimated)
    ANA_I_ERR,                  // Current regulation error
    ANA_V_REF,                  // Voltage reference
    ANA_V_REF_SAT,              // Voltage reference after magnet saturation compensation applied
    ANA_V_REF_LIMITED,          // Voltage reference after saturation compensation and limits applied
    ANA_V_MEAS,                 // Unfiltered measured voltage
    ANA_V_MEAS_FLTR,            // Filtered measured voltage
    ANA_V_ERR,                  // Voltage regulation error
    ANA_MAX_ABS_B_ERR,          // Max absolute field regulation error
    ANA_MAX_ABS_I_ERR,          // Max absolute current regulation error
    ANA_MAX_ABS_V_ERR,          // Max absolute voltage regulation error
    DIG_B_MEAS_TRIP,            // Field digital signals
    DIG_B_MEAS_LOW,
    DIG_B_MEAS_ZERO,
    DIG_B_REF_CLIP,
    DIG_B_REF_RATE_CLIP,
    DIG_B_REG_ERR_WARN,
    DIG_B_REG_ERR_FLT,
    DIG_I_MEAS_TRIP,            // Current digital signals
    DIG_I_MEAS_LOW,
    DIG_I_MEAS_ZERO,
    DIG_I_REF_CLIP,
    DIG_I_REF_RATE_CLIP,
    DIG_I_REG_ERR_WARN,
    DIG_I_REG_ERR_FLT,
    DIG_V_REF_CLIP,             // Voltage digital signals
    DIG_V_REF_RATE_CLIP,
    DIG_V_REG_ERR_WARN,
    DIG_V_REG_ERR_FLT,

    NUM_SIGNALS
};

// Signal names

struct signals
{
    char                       *name;
    enum ccsig_type             type;
    char                       *meta_data;
    enum fg_enabled_disabled    flag;
    float                       dig_offset;
    float                       value;
    char                       *cursor_label;
    float                      *buf;
};

FGSIGS_EXT struct signals signals[]
#ifdef GLOBALS
= {
    { "LOAD",                   CURSOR,         "CURSOR"     },
    { "REGMODE",                CURSOR,         "CURSOR"     },
    { "REF",                    CURSOR,         "CURSOR"     },
    { "B_REF",                  ANALOG,         "TRAIL_STEP" },
    { "B_REF_LIMITED",          ANALOG,         "TRAIL_STEP" },
    { "B_REF_RST",              ANALOG,         "TRAIL_STEP" },
    { "B_MEAS",                 ANALOG,         ""           },
    { "B_MEAS_FLTR",            ANALOG,         ""           },
    { "B_REG",                  ANALOG,         "TRAIL_STEP" },
    { "B_ERR",                  ANALOG,         "TRAIL_STEP" },
    { "I_REF",                  ANALOG,         "TRAIL_STEP" },
    { "I_REF_LIMITED",          ANALOG,         "TRAIL_STEP" },
    { "I_REF_RST",              ANALOG,         "TRAIL_STEP" },
    { "I_MEAS",                 ANALOG,         ""           },
    { "I_MEAS_FLTR",            ANALOG,         ""           },
    { "I_REG",                  ANALOG,         "TRAIL_STEP" },
    { "I_ERR",                  ANALOG,         "TRAIL_STEP" },
    { "V_REF",                  ANALOG,         "TRAIL_STEP" },
    { "V_REF_SAT",              ANALOG,         "TRAIL_STEP" },
    { "V_REF_LIMITED",          ANALOG,         "TRAIL_STEP" },
    { "V_MEAS",                 ANALOG,         ""           },
    { "V_MEAS_FLTR",            ANALOG,         ""           },
    { "V_ERR",                  ANALOG,         "TRAIL_STEP" },
    { "MAX_ABS_B_ERR",          ANALOG,         "TRAIL_STEP" },
    { "MAX_ABS_I_ERR",          ANALOG,         "TRAIL_STEP" },
    { "MAX_ABS_V_ERR",          ANALOG,         "TRAIL_STEP" },
    { "B_MEAS_TRIP",            DIGITAL,        "TRAIL_STEP" },
    { "B_MEAS_LOW",             DIGITAL,        "TRAIL_STEP" },
    { "B_MEAS_ZERO",            DIGITAL,        "TRAIL_STEP" },
    { "B_REF_CLIP",             DIGITAL,        "TRAIL_STEP" },
    { "B_REF_RATE_CLIP",        DIGITAL,        "TRAIL_STEP" },
    { "B_REG_ERR_WARN",         DIGITAL,        "TRAIL_STEP" },
    { "B_REG_ERR_FLT",          DIGITAL,        "TRAIL_STEP" },
    { "I_MEAS_TRIP",            DIGITAL,        "TRAIL_STEP" },
    { "I_MEAS_LOW",             DIGITAL,        "TRAIL_STEP" },
    { "I_MEAS_ZERO",            DIGITAL,        "TRAIL_STEP" },
    { "I_REF_CLIP",             DIGITAL,        "TRAIL_STEP" },
    { "I_REF_RATE_CLIP",        DIGITAL,        "TRAIL_STEP" },
    { "I_REG_ERR_WARN",         DIGITAL,        "TRAIL_STEP" },
    { "I_REG_ERR_FLT",          DIGITAL,        "TRAIL_STEP" },
    { "V_REF_CLIP",             DIGITAL,        "TRAIL_STEP" },
    { "V_REF_RATE_CLIP",        DIGITAL,        "TRAIL_STEP" },
    { "V_REG_ERR_WARN",         DIGITAL,        "TRAIL_STEP" },
    { "V_REG_ERR_FLT",          DIGITAL,        "TRAIL_STEP" },
}
#endif
;

// Function declarations

void    ccsigsPrepare           (void);
void    ccsigsStore             (float time);
void    ccsigsStoreCursor       (enum ccsig_idx idx, char *cursor_label);
void    ccsigsFlot              (void);

#endif
/*---------------------------------------------------------------------------------------------------------*\
  End of file: ccsigs.h
\*---------------------------------------------------------------------------------------------------------*/


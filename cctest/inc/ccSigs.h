/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/ccSigs.h                                                         Copyright CERN 2014

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

  Purpose:  Header file for cctest program output signals
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCSIGS_H
#define CCSIGS_H

#include <stdint.h>

#include "ccPars.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCSIGS_EXT
#else
#define CCSIGS_EXT extern
#endif

// Constants

#define DIG_STEP        0.5      // Digital signal step size

// Signal constants

enum ccsig_type
{
    ANALOG,
    DIGITAL,
    CURSOR
};

enum ccsig_idx
{
    CSR_FUNC,                   // Start of function cursor

    ANA_B_REF,                  // Field reference
    ANA_B_REF_LIMITED,          // Field reference after limits applied
    ANA_B_REF_RST,              // Field reference stored in RST history (limited and after back-calculation)
    ANA_B_REF_OPENLOOP,         // Field reference stored in open loop history (after back-calculation)
    ANA_B_REF_DELAYED,          // Field reference delayed

    ANA_B_MAGNET,               // Field in the magnet
    ANA_B_MEAS,                 // Unfiltered field measurement
    ANA_B_MEAS_FLTR,            // Filtered field measurement
    ANA_B_MEAS_EXTR,            // Extrapolated field measurement

    ANA_I_REF,                  // Current reference
    ANA_I_REF_LIMITED,          // Current reference after limits applied
    ANA_I_REF_RST,              // Current reference stored in RST history (limited and after back-calculation)
    ANA_I_REF_OPENLOOP,         // Current reference stored in open loop history (after back-calculation)
    ANA_I_REF_DELAYED,          // Current reference delayed

    ANA_I_MAGNET,               // Current in the magnet
    ANA_I_CIRCUIT,              // Current in the circuit
    ANA_I_RMS,                  // Filtered RMS circuit current for converter protection
    ANA_I_RMS_LOAD,             // Filtered RMS circuit current for load protections
    ANA_I_MEAS,                 // Unfiltered measured current
    ANA_I_MEAS_FLTR,            // Filtered measured current
    ANA_I_MEAS_EXTR,            // Extrapolated measured current

    ANA_REG_MEAS,               // Field or current measurement used for regulation

    ANA_V_REF,                  // Voltage reference
    ANA_V_REF_SAT,              // Voltage reference after magnet saturation compensation applied
    ANA_V_REF_LIMITED,          // Voltage reference after saturation compensation and limits applied
    ANA_V_CIRCUIT,              // Voltage across circuit
    ANA_V_MEAS,                 // Measured voltage

    ANA_TRACK_DLY,              // Measured track delay

    ANA_B_ERR,                  // Field regulation error
    ANA_I_ERR,                  // Current regulation error
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

    DIG_I_RMS_WARN,
    DIG_I_RMS_FLT,
    DIG_I_RMS_LOAD_WARN,
    DIG_I_RMS_LOAD_FLT,

    DIG_I_REF_CLIP,
    DIG_I_REF_RATE_CLIP,
    DIG_I_REG_ERR_WARN,
    DIG_I_REG_ERR_FLT,

    DIG_V_REF_CLIP,             // Voltage digital signals
    DIG_V_REF_RATE_CLIP,
    DIG_V_REG_ERR_WARN,
    DIG_V_REG_ERR_FLT,

    DIG_INVALID_MEAS,           // Invalid measurement

    NUM_SIGNALS
};

// Signal structure

struct signals
{
    char                       *name;                   // Signal name
    enum ccsig_type             type;                   // Signal type (CURSON, ANALOG, DIGITAL)
    char                       *meta_data;              // LVDV meta data (CURSOR, TRAIL_STEP)
    enum reg_enabled_disabled   control;                // Signal in use flag (ENABLED/DISABLED)
    float                       dig_offset;             // Digital trace offset
    float                       time_offset;            // Time offset for trace
    float                       value;                  // Signal value
    char                       *cursor_label;           // Cursor signal label
    float                      *buf;                    // Signal buffer (for FLOT output)
    uint32_t                    num_bad_values;         // Counter for bad values
};

CCSIGS_EXT struct signals signals[]     // IMPORTANT: This must be in the same order as enum ccsig_idx (above)
#ifdef GLOBALS
= {
    { "FUNCTION",               CURSOR,         "CURSOR"     },

    { "B_REF",                  ANALOG,         "TRAIL_STEP" },
    { "B_REF_LIMITED",          ANALOG,         "TRAIL_STEP" },
    { "B_REF_RST",              ANALOG,         "TRAIL_STEP" },
    { "B_REF_OPENLOOP",         ANALOG,         "TRAIL_STEP" },
    { "B_REF_DELAYED",          ANALOG,         "TRAIL_STEP" },

    { "B_MAGNET",               ANALOG,         "TRAIL_STEP" },
    { "B_MEAS",                 ANALOG,         ""           },
    { "B_MEAS_FLTR",            ANALOG,         ""           },
    { "B_MEAS_EXTR",            ANALOG,         ""           },

    { "I_REF",                  ANALOG,         "TRAIL_STEP" },
    { "I_REF_LIMITED",          ANALOG,         "TRAIL_STEP" },
    { "I_REF_RST",              ANALOG,         "TRAIL_STEP" },
    { "I_REF_OPENLOOP",         ANALOG,         "TRAIL_STEP" },
    { "I_REF_DELAYED",          ANALOG,         "TRAIL_STEP" },

    { "I_MAGNET",               ANALOG,         "TRAIL_STEP" },
    { "I_CIRCUIT",              ANALOG,         "TRAIL_STEP" },
    { "I_RMS",                  ANALOG,         ""           },
    { "I_RMS_LOAD",             ANALOG,         ""           },
    { "I_MEAS",                 ANALOG,         ""           },
    { "I_MEAS_FLTR",            ANALOG,         ""           },
    { "I_MEAS_EXTR",            ANALOG,         ""           },

    { "REG_MEAS",               ANALOG,         "TRAIL_STEP" },

    { "V_REF",                  ANALOG,         "TRAIL_STEP" },
    { "V_REF_SAT",              ANALOG,         "TRAIL_STEP" },
    { "V_REF_LIMITED",          ANALOG,         "TRAIL_STEP" },
    { "V_CIRCUIT",              ANALOG,         "TRAIL_STEP" },
    { "V_MEAS",                 ANALOG,         ""           },

    { "TRACK_DLY",              ANALOG,         "TRAIL_STEP" },

    { "B_ERR",                  ANALOG,         "TRAIL_STEP" },
    { "I_ERR",                  ANALOG,         "TRAIL_STEP" },
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

    { "I_RMS_WARN",             DIGITAL,        "TRAIL_STEP" },
    { "I_RMS_FLT",              DIGITAL,        "TRAIL_STEP" },
    { "I_RMS_LOAD_WARN",        DIGITAL,        "TRAIL_STEP" },
    { "I_RMS_LOAD_FLT",         DIGITAL,        "TRAIL_STEP" },

    { "I_REF_CLIP",             DIGITAL,        "TRAIL_STEP" },
    { "I_REF_RATE_CLIP",        DIGITAL,        "TRAIL_STEP" },
    { "I_REG_ERR_WARN",         DIGITAL,        "TRAIL_STEP" },
    { "I_REG_ERR_FLT",          DIGITAL,        "TRAIL_STEP" },

    { "V_REF_CLIP",             DIGITAL,        "TRAIL_STEP" },
    { "V_REF_RATE_CLIP",        DIGITAL,        "TRAIL_STEP" },
    { "V_REG_ERR_WARN",         DIGITAL,        "TRAIL_STEP" },
    { "V_REG_ERR_FLT",          DIGITAL,        "TRAIL_STEP" },

    { "INVALID_MEAS",           DIGITAL,        "TRAIL_STEP" },
}
#endif
;

// Function declarations

void     ccSigsInit              (void);
void     ccSigsStore             (double time);
void     ccSigsStoreCursor       (enum ccsig_idx idx, char *cursor_label);
uint32_t ccSigsReportBadValues   (void);

#endif
// EOF

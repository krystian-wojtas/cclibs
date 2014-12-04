/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/func/pulse.h                                                     Copyright CERN 2014

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

  Purpose:  Structure for PULSE function parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_PULSE_H
#define CCPARS_PULSE_H

#include "ccTest.h"
#include "ccPars.h"
#include "libfg/trim.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_PULSE_EXT
#else
#define CCPARS_PULSE_EXT extern
#endif

// Pulse data structure

struct ccpars_pulse
{
    // cctest PULSE parameters

    float                       time    [CC_NUM_CYC_SELS][1];     // Start of pulse time
    float                       duration[CC_NUM_CYC_SELS][1];     // Start of pulse time
    float                       ref     [CC_NUM_CYC_SELS][1];     // Pulse reference

    // Libfg TRIM variables

    struct fg_trim_pars         pars[CC_NUM_CYC_SELS];            // Libfg parameters for PULSE using Linear TRIM function
};

CCPARS_PULSE_EXT struct ccpars_pulse ccpars_pulse
#ifdef GLOBALS
= {// Default value                 Parameter
    { { 1.0 } },                 // PULSE TIME
    { { 1.0 } },                 // PULSE DURATION
    { { 0.0 } },                 // PULSE REF
}
#endif
;
// Pulse data description structure

CCPARS_PULSE_EXT struct ccpars   pulse_pars[]
#ifdef GLOBALS
= {// "Signal name", type,  max_n_els, *enum,        *value,                   num_defaults  flags
    { "TIME",        PAR_FLOAT, 1,      NULL, { .f = &ccpars_pulse.time    [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "DURATION",    PAR_FLOAT, 1,      NULL, { .f = &ccpars_pulse.duration[0][0] }, 1, PARS_CYCLE_SELECTOR },
    { "REF",         PAR_FLOAT, 1,      NULL, { .f = &ccpars_pulse.ref     [0][0] }, 1, PARS_CYCLE_SELECTOR },
    { NULL }
}
#endif
;

#endif
// EOF

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

// Libfg TRIM parameter structures for PULSE

CCPARS_PULSE_EXT struct fg_trim fg_pulse[CC_NUM_CYC_SELS];

// Pulse data structure

struct ccpars_pulse
{
    // cctest PULSE parameters

    float                       time;                           // Start of pulse time
    float                       duration;                       // Pulse duration
    float                       ref;                            // Pulse reference
};

CCPARS_PULSE_EXT struct ccpars_pulse ccpars_pulse[CC_NUM_CYC_SELS]
#ifdef GLOBALS
= {// Default value                 Parameter
    {   1.0,                     // PULSE TIME
        1.0,                     // PULSE DURATION
        0.0   },                 // PULSE REF
}
#endif
;
// Pulse data description structure

CCPARS_PULSE_EXT struct ccpars   pulse_pars[]
#ifdef GLOBALS
= {// "Signal name", type,  max_n_els, *enum,        *value,                num_defaults      cyc_sel_step      flags
    { "TIME",        PAR_FLOAT, 1,      NULL, { .f = &ccpars_pulse[0].time     }, 1, sizeof(struct ccpars_pulse), 0 },
    { "DURATION",    PAR_FLOAT, 1,      NULL, { .f = &ccpars_pulse[0].duration }, 1, sizeof(struct ccpars_pulse), 0 },
    { "REF",         PAR_FLOAT, 1,      NULL, { .f = &ccpars_pulse[0].ref      }, 1, sizeof(struct ccpars_pulse), 0 },
    { NULL }
}
#endif
;

#endif
// EOF

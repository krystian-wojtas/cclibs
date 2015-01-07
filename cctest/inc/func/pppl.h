/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/func/pppl.h                                                      Copyright CERN 2014

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

  Purpose:  Structure for PPPL function parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_PPPL_H
#define CCPARS_PPPL_H

#include "ccTest.h"
#include "ccPars.h"
#include "libfg/pppl.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_PPPL_EXT
#else
#define CCPARS_PPPL_EXT extern
#endif

// Libfg PPPL parameter structures

CCPARS_PPPL_EXT struct fg_pppl fg_pppl[CC_NUM_CYC_SELS];

// PPPL parameters structure

struct ccpars_pppl
{
    // cctest PPPL parameters

    float                       initial_ref;                    // Initial reference
    float                       acceleration1[FG_MAX_PPPLS];    // Acceleration of first  (parabolic) segment.
    float                       acceleration2[FG_MAX_PPPLS];    // Acceleration of second (parabolic) segment.
    float                       acceleration3[FG_MAX_PPPLS];    // Acceleration of third  (parabolic) segment.
    float                       rate2        [FG_MAX_PPPLS];    // Rate of change at start of second (parabolic) segment.
    float                       rate4        [FG_MAX_PPPLS];    // Rate of change of fourth (linear) segment.
    float                       ref4         [FG_MAX_PPPLS];    // Reference at start of fourth (linear) segment.
    float                       duration4    [FG_MAX_PPPLS];    // Duration of fourth (linear) segment.
};

CCPARS_PPPL_EXT struct ccpars_pppl ccpars_pppl[CC_NUM_CYC_SELS]
#ifdef GLOBALS
= {//   Default value           Parameter
    {    0.0,                // PPPL INITIAL_REF
      {  5.0 },              // PPPL ACCELERATION1
      { -0.1 },              // PPPL ACCELERATION2
      { -2.0 },              // PPPL ACCELERATION3
      {  1.0 },              // PPPL RATE2
      {  0.0 },              // PPPL RATE4
      {  1.0 },              // PPPL REF4
      {  0.1 } },            // PPPL DURATION4
}
#endif
;

// PPPL data description structure

CCPARS_PPPL_EXT struct ccpars   pppl_pars[]
#ifdef GLOBALS
= {// "Signal name"    type,         max_n_els, *enum,        *value,                    num_defaults      cyc_sel_step     flags
    { "INITIAL_REF",   PAR_FLOAT,            1,  NULL, { .f = &ccpars_pppl[0].initial_ref   }, 1, sizeof(struct ccpars_pppl), 0 },
    { "ACCELERATION1", PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl[0].acceleration1 }, 1, sizeof(struct ccpars_pppl), 0 },
    { "ACCELERATION2", PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl[0].acceleration2 }, 1, sizeof(struct ccpars_pppl), 0 },
    { "ACCELERATION3", PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl[0].acceleration3 }, 1, sizeof(struct ccpars_pppl), 0 },
    { "RATE2",         PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl[0].rate2         }, 1, sizeof(struct ccpars_pppl), 0 },
    { "RATE4",         PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl[0].rate4         }, 1, sizeof(struct ccpars_pppl), 0 },
    { "REF4",          PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl[0].ref4          }, 1, sizeof(struct ccpars_pppl), 0 },
    { "DURATION4",     PAR_FLOAT, FG_MAX_PPPLS,  NULL, { .f =  ccpars_pppl[0].duration4     }, 1, sizeof(struct ccpars_pppl), 0 },
    { NULL }
}
#endif
;

#endif
// EOF

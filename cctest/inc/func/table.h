/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/func/table.h                                                     Copyright CERN 2014

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

  Purpose:  Structure for TABLE function parameters

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_TABLE_H
#define CCPARS_TABLE_H

#include "ccTest.h"
#include "ccPars.h"
#include "libfg/table.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_TABLE_EXT
#else
#define CCPARS_TABLE_EXT extern
#endif

// Libfg TABLE parameter structures

CCPARS_TABLE_EXT struct fg_table fg_table[CC_NUM_CYC_SELS];

// Table data structure

#define TABLE_LEN       10000

struct ccpars_table
{
    // cctest TABLE parameters

    float                       ref [TABLE_LEN];                // Reference array
    float                       time[TABLE_LEN];                // Time array
};

CCPARS_TABLE_EXT struct ccpars_table ccpars_table[CC_NUM_CYC_SELS]
#ifdef GLOBALS
= {//     Default value                Parameter
    { { 0.0, 1.0, 1.0, 0.0 },       // TABLE REF
      { 0.0, 1.0, 2.0, 3.0 } },     // TABLE TIME
}
#endif
;
// Table data description structure

CCPARS_TABLE_EXT struct ccpars   table_pars[]
#ifdef GLOBALS
= {// "Signal name", type,      max_n_els, *enum,       *value,            num_defaults      cyc_sel_step      flags
    { "REF",         PAR_FLOAT, TABLE_LEN,  NULL, { .f = ccpars_table[0].ref  }, 4, sizeof(struct ccpars_table), 0 },
    { "TIME",        PAR_FLOAT, TABLE_LEN,  NULL, { .f = ccpars_table[0].time }, 4, sizeof(struct ccpars_table), 0 },
    { NULL }
}
#endif
;

#endif
// EOF

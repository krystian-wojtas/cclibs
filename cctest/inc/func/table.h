/*---------------------------------------------------------------------------------------------------------*\
  File:     func/table.h                                                                Copyright CERN 2011

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

  Purpose:  Structure for the table data file (-d table_file)

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCPARS_TABLE_H
#define CCPARS_TABLE_H

#include "libfg/table.h"

// GLOBALS is defined in source file where global variables should be defined

#ifdef GLOBALS
#define CCPARS_TABLE_EXT
#else
#define CCPARS_TABLE_EXT extern
#endif

// Table data structure

#define TABLE_LEN       10000

struct ccpars_table
{
    // Table file data

    float       ref [TABLE_LEN];
    float       time[TABLE_LEN];

    // Libfg table variables

    struct fg_table_config      config;                 // Libfg config struct for TABLE

    struct fg_table_pars        table_pars;             // Libfg parameters for TABLE
};

CCPARS_TABLE_EXT struct ccpars_table ccpars_table;

// Table data description structure

CCPARS_TABLE_EXT struct ccpars   table_pars_list[]
#ifdef GLOBALS
= {// "Signal name", TYPE,       max_vals, min_vals, *enum,  *value,             num_defaults
    { "REF",         PAR_FLOAT, TABLE_LEN, 2, NULL,        { .f = ccpars_table.ref  }, 0 },
    { "TIME",        PAR_FLOAT, TABLE_LEN, 2, NULL,        { .f = ccpars_table.time }, 0 },
    { NULL }
}
#endif
;

#endif
/*---------------------------------------------------------------------------------------------------------*\
  End of file: func/table.h
\*---------------------------------------------------------------------------------------------------------*/


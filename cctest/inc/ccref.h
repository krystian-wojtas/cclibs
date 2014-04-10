/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/ccref.h                                                          Copyright CERN 2014

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

  Purpose:  Header file for FG library test program reference generation functions

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCREF_H
#define CCREF_H

#include "cccmds.h"

// Include fg and reg library header files

#include "libfg.h"
#include "libfg/plep.h"
#include "libfg/ramp.h"
#include "libfg/pppl.h"
#include "libfg/table.h"
#include "libfg/test.h"
#include "libfg/trim.h"

// Include start function header

#include "func/start.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCREF_EXT
#else
#define CCREF_EXT extern
#endif

// Function prototypes

void            ccrefFuncType           (char *arg);
void            ccrefInitSTART          (void);
void            ccrefInitPLEP           (void);
void            ccrefInitRAMP           (void);
void            ccrefInitPPPL           (void);
void            ccrefInitTABLE          (void);
void            ccrefInitSTEPS          (void);
void            ccrefInitSQUARE         (void);
void            ccrefInitSINE           (void);
void            ccrefInitCOSINE         (void);
void            ccrefInitLTRIM          (void);
void            ccrefInitCTRIM          (void);

uint32_t        ccrefStartGen           (struct fg_plep_pars *pars, const double *time, float *ref);

enum fg_error   ccrefCheckConverterLimits (struct fg_limits *limits, uint32_t invert_limits, 
                                           float ref, float rate, float acceleration);

// Function meta data

CCREF_EXT struct fg_meta fg_meta[MAX_FUNCS];

// Reference functions structure

struct fgfunc
{
    enum cccmds_enum         cmd_idx;
    void                    *fg_pars;
    void                    (*init_func)(void);
    uint32_t                (*fgen_func)();
};

CCREF_EXT struct fgfunc func[]  // Must be in enum fg_types order (in global.h)!
#ifdef GLOBALS
= {
    {   0,         NULL,                     NULL,               NULL          },
    {   CMD_START, &ccpars_start.plep_pars,  ccrefInitSTART,     ccrefStartGen },
    {   CMD_PLEP,  &ccpars_plep.plep_pars,   ccrefInitPLEP,      fgPlepGen     },
    {   CMD_RAMP,  &ccpars_ramp.ramp_pars,   ccrefInitRAMP,      fgRampGen     },
    {   CMD_PPPL,  &ccpars_pppl.pppl_pars,   ccrefInitPPPL,      fgPpplGen     },
    {   CMD_TABLE, &ccpars_table.table_pars, ccrefInitTABLE,     fgTableGen    },
    {   CMD_TEST,  &ccpars_test.test_pars,   ccrefInitSTEPS,     fgTestGen     },
    {   CMD_TEST,  &ccpars_test.test_pars,   ccrefInitSQUARE,    fgTestGen     },
    {   CMD_TEST,  &ccpars_test.test_pars,   ccrefInitSINE,      fgTestGen     },
    {   CMD_TEST,  &ccpars_test.test_pars,   ccrefInitCOSINE,    fgTestGen     },
    {   CMD_TRIM,  &ccpars_trim.trim_pars,   ccrefInitLTRIM,     fgTrimGen     },
    {   CMD_TRIM,  &ccpars_trim.trim_pars,   ccrefInitCTRIM,     fgTrimGen     },
}
#endif
;
#endif
// EOF

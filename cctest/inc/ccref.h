/*---------------------------------------------------------------------------------------------------------*\
  File:     ccref.h                                                                     Copyright CERN 2011

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

#ifndef FGREF_H
#define FGREF_H

// Include fg and reg library header files

#include "libfg.h"
#include "libfg/plep.h"
#include "libfg/ramp.h"
#include "libfg/pppl.h"
#include "libfg/spline.h"
#include "libfg/table.h"
#include "libfg/test.h"
#include "libfg/trim.h"

// Include start function header

#include "func/start.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define FGREF_EXT
#else
#define FGREF_EXT extern
#endif

// Function prototypes

void            ccrefFuncType           (char *arg);
void            ccrefInitSTART          (void);
void            ccrefInitPLEP           (void);
void            ccrefInitRAMP           (void);
void            ccrefInitPPPL           (void);
void            ccrefInitSPLINE         (void);
void            ccrefInitTABLE          (void);
void            ccrefInitSTEPS          (void);
void            ccrefInitSQUARE         (void);
void            ccrefInitSINE           (void);
void            ccrefInitCOSINE         (void);
void            ccrefInitLTRIM          (void);
void            ccrefInitCTRIM          (void);

uint32_t        ccrefStartGen           (struct fg_plep_pars *pars, const double *time, float *ref);

enum fg_error   ccrefCheckConverterLimits (struct fg_limits *limits, enum fg_limits_polarity limits_polarity,
                                           uint32_t negative_flag, float ref, float rate, float acceleration);

// Function meta data

FGREF_EXT struct fg_meta fg_meta;

// Reference functions structure

struct fgfunc
{
    struct ccpars       *pars;
    void                *fg_pars;
    void                (*init_func)(void);
    uint32_t            (*fgen_func)();
};

FGREF_EXT struct fgfunc func[]  // Must be in enum fg_types order!
#ifdef GLOBALS
= {
    {   NULL,         NULL,                        NULL,               NULL          },
    {   start_pars,   &ccpars_start.plep_pars,     ccrefInitSTART,     ccrefStartGen },
    {   plep_pars,    &ccpars_plep.plep_pars,      ccrefInitPLEP,      fgPlepGen     },
    {   ramp_pars,    &ccpars_ramp.ramp_pars,      ccrefInitRAMP,      fgRampGen     },
    {   pppl_pars,    &ccpars_pppl.pppl_pars,      ccrefInitPPPL,      fgPpplGen     },
    {   table_pars,   &ccpars_table.spline_pars,   ccrefInitSPLINE,    fgSplineGen   },
    {   table_pars,   &ccpars_table.table_pars,    ccrefInitTABLE,     fgTableGen    },
    {   test_pars,    &ccpars_test.test_pars,      ccrefInitSTEPS,     fgTestGen     },
    {   test_pars,    &ccpars_test.test_pars,      ccrefInitSQUARE,    fgTestGen     },
    {   test_pars,    &ccpars_test.test_pars,      ccrefInitSINE,      fgTestGen     },
    {   test_pars,    &ccpars_test.test_pars,      ccrefInitCOSINE,    fgTestGen     },
    {   trim_pars,    &ccpars_trim.trim_pars,      ccrefInitLTRIM,     fgTrimGen     },
    {   trim_pars,    &ccpars_trim.trim_pars,      ccrefInitCTRIM,     fgTrimGen     },
}
#endif
;
#endif
/*---------------------------------------------------------------------------------------------------------*\
  End of file: ccref.h
\*---------------------------------------------------------------------------------------------------------*/


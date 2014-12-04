/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/ccRef.h                                                          Copyright CERN 2014

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

#include "ccCmds.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCREF_EXT
#else
#define CCREF_EXT extern
#endif

// Function prototypes

bool            ccRefDirectGen          (struct fg_table_pars *pars, const double *time, float *ref);

uint32_t        ccRefInitDIRECT         (struct fg_meta *fg_meta, uint32_t cyc_sel);
uint32_t        ccRefInitPLEP           (struct fg_meta *fg_meta, uint32_t cyc_sel);
uint32_t        ccRefInitRAMP           (struct fg_meta *fg_meta, uint32_t cyc_sel);
uint32_t        ccRefInitPPPL           (struct fg_meta *fg_meta, uint32_t cyc_sel);
uint32_t        ccRefInitTABLE          (struct fg_meta *fg_meta, uint32_t cyc_sel);
uint32_t        ccRefInitSTEPS          (struct fg_meta *fg_meta, uint32_t cyc_sel);
uint32_t        ccRefInitSQUARE         (struct fg_meta *fg_meta, uint32_t cyc_sel);
uint32_t        ccRefInitSINE           (struct fg_meta *fg_meta, uint32_t cyc_sel);
uint32_t        ccRefInitCOSINE         (struct fg_meta *fg_meta, uint32_t cyc_sel);
uint32_t        ccRefInitLTRIM          (struct fg_meta *fg_meta, uint32_t cyc_sel);
uint32_t        ccRefInitCTRIM          (struct fg_meta *fg_meta, uint32_t cyc_sel);
uint32_t        ccRefInitPULSE          (struct fg_meta *fg_meta, uint32_t cyc_sel);

// Reference functions structure

struct fgfunc
{
    enum cccmds_enum         cmd_idx;
    char                    *fg_pars;
    size_t                   size_of_pars;
    enum fg_error           (*init_func)(struct fg_meta *fg_meta, uint32_t cyc_sel);
    bool                    (*fgen_func)();
};

CCREF_EXT struct fgfunc funcs[]  // Must be in enum fg_types order (in ref.h)
#ifdef GLOBALS
= {
    {   0,         NULL,                       0,                            NULL,            NULL           },
    {   CMD_TABLE, (char *)&ccpars_table.pars, sizeof(struct fg_table_pars), ccRefInitTABLE,  ccRefDirectGen },
    {   CMD_PLEP,  (char *)&ccpars_plep.pars,  sizeof(struct fg_plep_pars),  ccRefInitPLEP,   fgPlepGen      },
    {   CMD_RAMP,  (char *)&ccpars_ramp.pars,  sizeof(struct fg_ramp_pars),  ccRefInitRAMP,   fgRampGen      },
    {   CMD_PPPL,  (char *)&ccpars_pppl.pars,  sizeof(struct fg_pppl_pars),  ccRefInitPPPL,   fgPpplGen      },
    {   CMD_TABLE, (char *)&ccpars_table.pars, sizeof(struct fg_table_pars), ccRefInitTABLE,  fgTableGen     },
    {   CMD_TEST,  (char *)&ccpars_test.pars,  sizeof(struct fg_test_pars),  ccRefInitSTEPS,  fgTestGen      },
    {   CMD_TEST,  (char *)&ccpars_test.pars,  sizeof(struct fg_test_pars),  ccRefInitSQUARE, fgTestGen      },
    {   CMD_TEST,  (char *)&ccpars_test.pars,  sizeof(struct fg_test_pars),  ccRefInitSINE,   fgTestGen      },
    {   CMD_TEST,  (char *)&ccpars_test.pars,  sizeof(struct fg_test_pars),  ccRefInitCOSINE, fgTestGen      },
    {   CMD_TRIM,  (char *)&ccpars_trim.pars,  sizeof(struct fg_trim_pars),  ccRefInitLTRIM,  fgTrimGen      },
    {   CMD_TRIM,  (char *)&ccpars_trim.pars,  sizeof(struct fg_trim_pars),  ccRefInitCTRIM,  fgTrimGen      },
    {   CMD_TRIM,  (char *)&ccpars_pulse.pars, sizeof(struct fg_trim_pars),  ccRefInitPULSE,  fgTrimGen      },
}
#endif
;
#endif
// EOF

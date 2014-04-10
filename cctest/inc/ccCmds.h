/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/cccmds.h                                                         Copyright CERN 2014

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

  Purpose:  Header file for cccmds.c

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCCMDS_H
#define CCCMDS_H

#include <stdint.h>

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCCMDS_EXT
#else
#define CCCMDS_EXT extern
#endif

// Constants

#define CC_MAX_FILE_LINE_LEN  65536
#define CC_PROMPT             ">"

// Include ccpars.h to declare

#include <ccpars.h>

// Include libreg header files - required by cccmds[]

#include "libreg.h"

// Include cctest function data header files

#include "func/start.h"
#include "func/plep.h"
#include "func/ramp.h"
#include "func/pppl.h"
#include "func/table.h"
#include "func/trim.h"
#include "func/test.h"

// Include cctest parameter header files - required by cccmds[]

#include "pars/global.h"
#include "pars/limits.h"
#include "pars/load.h"
#include "pars/meas.h"
#include "pars/reg.h"
#include "pars/vs.h"

// Function declarations

uint32_t cccmdsHelp  (uint32_t cmd_idx, char *pars);
uint32_t cccmdsLs    (uint32_t cmd_idx, char *pars);
uint32_t cccmdsCd    (uint32_t cmd_idx, char *pars);
uint32_t cccmdsRead  (uint32_t cmd_idx, char *pars);
uint32_t cccmdsSave  (uint32_t cmd_idx, char *pars);
uint32_t cccmdsDebug (uint32_t cmd_idx, char *pars);
uint32_t cccmdsRun   (uint32_t cmd_idx, char *pars);
uint32_t cccmdsPrint (uint32_t cmd_idx, char *pars);

// Command indexes - the order of enum cccmds_enum must match the cmds[] array below

enum cccmds_enum
{
    /* Global parameters */
    CMD_GLOBAL,    CMD_LIMITS,    CMD_LOAD,    CMD_MEAS,    CMD_REG_B,    CMD_REG_I,    CMD_VS,
    /* Function parameters */
    CMD_START,    CMD_PLEP,    CMD_RAMP,    CMD_PPPL,    CMD_TABLE,    CMD_TRIM,    CMD_TEST,
    /* Commands */    CMD_HELP,    CMD_LS,    CMD_CD,    CMD_READ,    CMD_SAVE,    CMD_DEBUG,    CMD_RUN,

    N_CMDS};// Define array of commands

struct cccmds
{
    char                *name;
    void                (*cmd_func)(uint32_t cmd_idx, char *par);
    struct ccpars       *pars;
    char                *help_message;
    uint32_t            n_pars_missing;
    uint32_t            enabled;
};

CCCMDS_EXT struct cccmds cmds[] // The order must match enum cccmds_enum (above)
#ifdef GLOBALS
= {
    { "Global", cccmdsPrint,  global_pars, "- print or set GLOBAL parameter(s)" },
    { "LImits", cccmdsPrint,  limits_pars, "- print or set LIMITS parameter(s)" },
    { "LOad",   cccmdsPrint,  load_pars  , "- print or set LOAD parameter(s)" },
    { "Meas",   cccmdsPrint,  meas_pars  , "- print or set MEAS parameter(s)" },
    { "Breg",   cccmdsPrint,  breg_pars  , "- print or set BREG parameter(s)" },
    { "Ireg",   cccmdsPrint,  ireg_pars  , "- print or set IREG parameter(s)" },
    { "Vs",     cccmdsPrint,  vs_pars,   , "- print or set VS parameter(s)" },
    { "STart",  cccmdsPrint,  start_pars , "- print or set START function parameter(s)" },
    { "PLep",   cccmdsPrint,  plep_pars  , "- print or set PLEP function parameter(s)" },
    { "RAmp",   cccmdsPrint,  ramp_pars  , "- print or set RAMP function parameter(s)" },
    { "PPpl",   cccmdsPrint,  pppl_pars  , "- print or set PPPL function parameter(s)" },
    { "TAble",  cccmdsPrint,  table_pars , "- print or set TABLE function parameter(s)" },
    { "TRim",   cccmdsPrint,  trim_pars  , "- print or set TRIM function parameter(s)" },
    { "TEst",   cccmdsPrint,  test_pars  , "- print or set TEST function parameter(s)" },
    { "Help",   cccmdsHelp ,  NULL       , "- print this help message" },
    { "LS",     cccmdsLs   ,  NULL       , "- list contents of current directory" },
    { "Cd",     cccmdsCd   ,  NULL       , "path - change current directory" },
    { "REad",   cccmdsRead ,  NULL       , "filename - read parameters from named file" },
    { "SAve",   cccmdsSave ,  NULL       , "filename - save all parameters in named file" },
    { "Debug",  cccmdsDebug,  NULL       , "- print all debug variables" },
    { "RUn",    cccmdsRun  ,  NULL       , "- run function generation test or converter simulation" },
    { NULL }
}
#endif
;

#endif
// EOF

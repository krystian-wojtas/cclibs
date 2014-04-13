/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/ccCmds.h                                                         Copyright CERN 2014

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
#define CC_MAX_CMD_NAME_LEN   6             //  Current longest command name
#define CC_PROMPT             ">"

// Include ccPars.h to declare

#include <ccPars.h>

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

uint32_t ccCmdsHelp  (uint32_t cmd_idx, char **remaining_line);
uint32_t ccCmdsLs    (uint32_t cmd_idx, char **remaining_line);
uint32_t ccCmdsCd    (uint32_t cmd_idx, char **remaining_line);
uint32_t ccCmdsPwd   (uint32_t cmd_idx, char **remaining_line);
uint32_t ccCmdsRead  (uint32_t cmd_idx, char **remaining_line);
uint32_t ccCmdsSave  (uint32_t cmd_idx, char **remaining_line);
uint32_t ccCmdsDebug (uint32_t cmd_idx, char **remaining_line);
uint32_t ccCmdsRun   (uint32_t cmd_idx, char **remaining_line);
uint32_t ccCmdsPar   (uint32_t cmd_idx, char **remaining_line);
uint32_t ccCmdsExit  (uint32_t cmd_idx, char **remaining_line);

// Command indexes - the order of enum cccmds_enum must match the cmds[] array below

enum cccmds_enum
{
    /* Global parameters */
    CMD_GLOBAL,    CMD_LIMITS,    CMD_LOAD,    CMD_MEAS,    CMD_REG_B,    CMD_REG_I,    CMD_VS,
    /* Function parameters */
    CMD_START,    CMD_PLEP,    CMD_RAMP,    CMD_PPPL,    CMD_TABLE,    CMD_TRIM,    CMD_TEST,
    /* Commands */    CMD_HELP,    CMD_LS,    CMD_CD,
    CMD_PWD,
    CMD_READ,    CMD_SAVE,    CMD_DEBUG,    CMD_RUN,
    CMD_EXIT,
    CMD_QUIT,

    N_CMDS};// Define array of commands

struct cccmds
{
    char                *name;
    uint32_t           (*cmd_func)(uint32_t cmd_idx, char **remaining_line);
    struct ccpars       *pars;
    char                *help_message;
    uint32_t            n_pars_missing;
    uint32_t            enabled;
};

CCCMDS_EXT struct cccmds cmds[] // The order must match enum cccmds_enum (above)
#ifdef GLOBALS
= {
    { "Global", ccCmdsPar  ,  global_pars, "[par [value][,value]...]  Print or set GLOBAL parameter(s)" },
    { "LImits", ccCmdsPar  ,  limits_pars, "[par [value][,value]...]  Print or set LIMITS parameter(s)" },
    { "LOad",   ccCmdsPar  ,  load_pars  , "[par [value][,value]...]  Print or set LOAD parameter(s)" },
    { "Meas",   ccCmdsPar  ,  meas_pars  , "[par [value][,value]...]  Print or set MEAS parameter(s)" },
    { "Breg",   ccCmdsPar  ,  breg_pars  , "[par [value][,value]...]  Print or set BREG parameter(s)" },
    { "Ireg",   ccCmdsPar  ,  ireg_pars  , "[par [value][,value]...]  Print or set IREG parameter(s)" },
    { "Vs",     ccCmdsPar  ,  vs_pars    , "[par [value][,value]...]  Print or set VS parameter(s)" },
    { "STart",  ccCmdsPar  ,  start_pars , "[par [value][,value]...]  Print or set START function parameter(s)" },
    { "PLep",   ccCmdsPar  ,  plep_pars  , "[par [value][,value]...]  Print or set PLEP function parameter(s)" },
    { "RAmp",   ccCmdsPar  ,  ramp_pars  , "[par [value][,value]...]  Print or set RAMP function parameter(s)" },
    { "PPpl",   ccCmdsPar  ,  pppl_pars  , "[par [value][,value]...]  Print or set PPPL function parameter(s)" },
    { "TAble",  ccCmdsPar  ,  table_pars , "[par [value][,value]...]  Print or set TABLE function parameter(s)" },
    { "TRim",   ccCmdsPar  ,  trim_pars  , "[par [value][,value]...]  Print or set TRIM function parameter(s)" },
    { "TEst",   ccCmdsPar  ,  test_pars  , "[par [value][,value]...]  Print or set TEST function parameter(s)" },
    { "Help",   ccCmdsHelp ,  NULL       , "                          Print this help message" },
    { "LS",     ccCmdsLs   ,  NULL       , "                          List contents of current directory" },
    { "Cd",     ccCmdsCd   ,  NULL       , "path                      Change current directory" },
    { "PWd",    ccCmdsPwd  ,  NULL       , "                          Print current directory" },
    { "REad",   ccCmdsRead ,  NULL       , "[filename]                Read parameters from named file or from stdin" },
    { "SAve",   ccCmdsSave ,  NULL       , "filename                  Save all parameters in named file" },
    { "Debug",  ccCmdsDebug,  NULL       , "                          Print all debug variables" },
    { "RUn",    ccCmdsRun  ,  NULL       , "                          Run function generation test or converter simulation" },
    { "Exit",   ccCmdsExit ,  NULL       , "                          Exit from program" },
    { "Quit",   ccCmdsExit ,  NULL       , "                          Exit from program" },
    { NULL }
}
#endif
;

#endif
// EOF

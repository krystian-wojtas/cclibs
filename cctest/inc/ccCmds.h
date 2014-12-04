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
#include <stdbool.h>

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCCMDS_EXT
#else
#define CCCMDS_EXT extern
#endif

// Constants

#define CC_MAX_FILE_LINE_LEN  65536
#define CC_MAX_CMD_NAME_LEN   7             //  Current longest command name
#define CC_MAX_PAR_NAME_LEN   34            //  Current longest parameter name
#define CC_PROMPT             ">"

// Include ccPars.h

#include <ccPars.h>

// Include libreg header files - required by cccmds[]

#include "libreg.h"

// Include cctest function data header files

#include "func/plep.h"
#include "func/pppl.h"
#include "func/pulse.h"
#include "func/ramp.h"
#include "func/table.h"
#include "func/test.h"
#include "func/trim.h"

// Include cctest parameter header files - required by cccmds[]

#include "pars/global.h"
#include "pars/limits.h"
#include "pars/load.h"
#include "pars/meas.h"
#include "pars/reg.h"
#include "pars/vs.h"
#include "pars/ref.h"
#include "pars/default.h"

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
uint32_t ccCmdsQuit  (uint32_t cmd_idx, char **remaining_line);

// Command indexes - the order of enum cccmds_enum must match the cmds[] array below

enum cccmds_enum
{
    /* Global parameters */
    CMD_GLOBAL,
    CMD_DEFAULT,    CMD_LIMITS,    CMD_LOAD,    CMD_MEAS,    CMD_BREG,    CMD_IREG,    CMD_VS,
    CMD_REF,
    /* Function parameters */
    CMD_PLEP,    CMD_PPPL,    CMD_PULSE,
    CMD_RAMP,
    CMD_TABLE,    CMD_TEST,
    CMD_TRIM,
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
    bool                is_enabled;
};

CCCMDS_EXT struct cccmds cmds[] // The order must match enum cccmds_enum (above)
#ifdef GLOBALS
= {
	// Global parameters
    { "GLOBAL",  ccCmdsPar  , global_pars , "           Print or set GLOBAL parameter(s)"                       },
    { "DEFAULT", ccCmdsPar  , default_pars, "           Print or set DEFAULT parameter(s)"                      },
    { "LIMITS",  ccCmdsPar  , limits_pars , "           Print or set LIMITS parameter(s)"                       },
    { "LOAD",    ccCmdsPar  , load_pars   , "           Print or set LOAD parameter(s)"                         },
    { "MEAS",    ccCmdsPar  , meas_pars   , "           Print or set MEAS parameter(s)"                         },
    { "BREG",    ccCmdsPar  , breg_pars   , "           Print or set BREG parameter(s)"                         },
    { "IREG",    ccCmdsPar  , ireg_pars   , "           Print or set IREG parameter(s)"                         },
    { "VS",      ccCmdsPar  , vs_pars     , "           Print or set VS parameter(s)"                           },
    { "REF",     ccCmdsPar  , ref_pars    , "           Print or set REF parameter(s)"                          },
    // Function parameters
    { "PLEP",    ccCmdsPar  , plep_pars   , "           Print or set PLEP function parameter(s)"                },
    { "PPPL",    ccCmdsPar  , pppl_pars   , "           Print or set PPPL function parameter(s)"                },
    { "PULSE",   ccCmdsPar  , pulse_pars  , "           Print or set PULSE function parameter(s)"               },
    { "RAMP",    ccCmdsPar  , ramp_pars   , "           Print or set RAMP function parameter(s)"                },
    { "TABLE",   ccCmdsPar  , table_pars  , "           Print or set TABLE function parameter(s)"               },
    { "TEST",    ccCmdsPar  , test_pars   , "           Print or set TEST function parameter(s)"                },
    { "TRIM",    ccCmdsPar  , trim_pars   , "           Print or set TRIM function parameter(s)"                },
    // Commands
    { "HELP",    ccCmdsHelp , NULL        , "           Print this help message"                                },
    { "LS",      ccCmdsLs   , NULL        , "           List contents of current directory"                     },
    { "CD",      ccCmdsCd   , NULL        , "path       Change current directory"                               },
    { "PWD",     ccCmdsPwd  , NULL        , "           Print current directory"                                },
    { "READ",    ccCmdsRead , NULL        , "[filename] Read parameters from named file or from stdin"          },
    { "SAVE",    ccCmdsSave , NULL        , "filename   Save all parameters in named file"                      },
    { "DEBUG",   ccCmdsDebug, NULL        , "           Print all debug variables"                              },
    { "RUN",     ccCmdsRun  , NULL        , "           Run function generation test or converter simulation"   },
    { "EXIT",    ccCmdsExit , NULL        , "           Exit from current file or quit when from stdin"         },
    { "QUIT",    ccCmdsQuit , NULL        , "           Quit program immediately"                               },
    { NULL }
}
#endif
;

#endif
// EOF

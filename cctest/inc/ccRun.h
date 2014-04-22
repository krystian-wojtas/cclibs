/*---------------------------------------------------------------------------------------------------------*\
  File:     cctest/inc/ccRun.h                                                          Copyright CERN 2014

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

  Purpose:  Header file for ccrun.c

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#ifndef CCRUN_H
#define CCRUN_H

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCRUN_EXT
#else
#define CCRUN_EXT extern
#endif

// Regulation related variables

struct ccrun_vars
{
    uint32_t            num_iterations;             // Number of iterations for the simulation
    double              reg_time;                   // Time of last iteration when regulation algorithm ran
    float               feedforward_v_ref;          // Feed-forward reference for START function
    uint32_t            feedforward_control;        // Feed-forward control for START function
    uint32_t            ireg_flag;                  // Run includes current regulation
    uint32_t            breg_flag;                  // Run includes field regulation
    uint32_t            func_idx;                   // Function index
    uint32_t            num_functions;              // Number of functions to run
    struct fg_limits   *fg_limits;                  // Pointer to fg_limits (b/i/v)
    struct reg_lim_ref  fg_lim_v_ref;               // Libreg voltage measurement limits structure for fg converter limits
    struct fg_meta      fg_meta[MAX_FUNCS];         // Reference function meta data for all functions
    double              ref_advance[MAX_FUNCS];     // Ref advance used with each function (for debugging)
};

CCRUN_EXT struct ccrun_vars ccrun;

// Function prototypes

void    ccRunSimulation         (void);
void    ccRunFuncGen            (void);
void    ccRunFuncGenReverseTime (void);

#endif
// EOF

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
    uint32_t                num_iterations;                     // Number of iterations for the simulation
    uint32_t                ireg_flag;                          // Run includes current regulation
    uint32_t                breg_flag;                          // Run includes field regulation
    uint32_t                vs_tripped_flag;                    // Voltage source tripped by measurement limit
    uint32_t                pre_func_idx;                       // Pre-function stage index (0,1,2,3)
    uint32_t                num_prefunc_ramps;                  // Number of pre-function ramps
    float                   pre_func_final_ref[MAX_PREFUNCS];

    uint32_t                func_idx;                           // Active reference function index
    uint32_t                num_functions;                      // Number of reference functions to run
    double                  func_start_time;                    // Start time (iter_time) for current function
    double                  func_duration;                      // Function duration including run delay
    bool                  (*fgen_func)();                       // Function to generate the active reference
    void                   *fg_pars;                            // Parameter structure for active reference
    struct fg_limits       *fg_limits;                          // Pointer to fg_limits (b/i/v)
    struct reg_lim_ref      fg_lim_v_ref;                       // Libreg voltage measurement limits structure for fg converter limits

    struct ccrun_func_log
    {
        double              func_start_time;                    // Time in iteration of the start of the function
        struct fg_meta      fg_meta;                            // Reference function meta data for all functions
        float               ref_advance;                        // Ref advance used with each function (for debugging)
        float               max_abs_err;                        // Max absolute regulation for each function (when regulation is active)
    } func[MAX_FUNCS];
};

CCRUN_EXT struct ccrun_vars ccrun;

// Function prototypes

void    ccRunSimulation         (void);
void    ccRunFuncGen            (void);
void    ccRunFuncGenReverseTime (void);

#endif
// EOF

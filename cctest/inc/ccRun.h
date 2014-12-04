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

#include "pars/global.h"

// GLOBALS should be defined in the source file where global variables should be defined

#ifdef GLOBALS
#define CCRUN_EXT
#else
#define CCRUN_EXT extern
#endif

// Constants

#define MAX_PREFUNCS        3

// Regulation related variables

struct ccrun_vars
{
    uint32_t                        num_iterations;                     // Number of iterations for the simulation
    uint32_t                        num_cycles;                         // Number of cycles defined in GLOBAL CYCLE_SELECTOR
    uint32_t                        cycle_idx;                          // Active reference function index

    struct fg_meta                  fg_meta[CC_NUM_CYC_SELS];           // Reference function meta data for all functions

    bool                            is_used[CC_NUM_CYC_SELS];           // Cycle selector is used by GLOBAL CYCLE_SELECTOR
    bool                            is_ireg_enabled;                    // Run includes current regulation
    bool                            is_breg_enabled;                    // Run includes field regulation
    bool                            is_vs_tripped;                      // Voltage source is tripped by measurement limit
    bool                            is_test_select_used;                // LOAD TEST_SELECT is used during the simulation

    double                          func_start_time;                    // Start time (iter_time) for current function
    double                          func_duration;                      // Function duration including run delay
    bool                          (*fgen_func)();                       // Function to generate the active reference
    void                           *fgen_pars;                          // Parameter structure for active reference
    struct fg_limits               *fg_limits;                          // Pointer to NULL or ccrun.fgen_limits
    struct fg_limits                fgen_limits;                        // Pointer to fg_limits (b/i/v)
    struct reg_lim_ref              fg_lim_v_ref;                       // Libreg voltage measurement limits structure for fg converter limits

    struct ccrun_prefunc
    {
        uint32_t                    idx;                                // Pre-function stage index (0,1,2,3)
        uint32_t                    num_ramps;                          // Number of pre-function ramps
        float                       final_ref[MAX_PREFUNCS];            // Final ref for each pre-function ramp
        struct fg_ramp_config       config;                             // Libfg parameters for pre-function ramps
        struct fg_ramp_pars         pars;                               // Libfg parameters for pre-function ramps
    } prefunc;

    struct ccrun_dyn_eco
    {
        struct fg_plep_config       config;                             // Dynamic economy plep configuration
        struct fg_plep_pars         pars;                               // Dynamic economy plep parameters
        bool                      (*fgen_func)();                       // Storage of pointer to function that was active (NULL when dyn_eco not active)
        void                       *fgen_pars;                          // Storage of pointer to parameters for function that was active

        struct ccrun_dyn_eco_log
        {
            int32_t                 length;                             // Number of points logged in dyn_eco.log
            float                   time[2 * MAX_PREFUNCS];             // Start/end time of dynamic economy window
            float                   ref [2 * MAX_PREFUNCS];             // Start/end reference of dynamic economy
        } log;
    } dyn_eco;

    struct ccrun_func_log
    {
        double                      func_start_time;                    // Time in iteration of the start of the function
        float                       ref_advance;                        // Ref advance used with each function (for debugging)
        float                       max_abs_err;                        // Max absolute regulation for each function (when regulation is active)
    } func[MAX_CYCLES];
};

CCRUN_EXT struct ccrun_vars ccrun;

// Function prototypes

void    ccRunSimulation         (void);
void    ccRunFuncGen            (void);
void    ccRunFuncGenReverseTime (void);

#endif
// EOF

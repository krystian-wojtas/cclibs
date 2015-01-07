/*---------------------------------------------------------------------------------------------------------*\
  File:     ccRef.c                                                                     Copyright CERN 2014

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

  Purpose:  Function generation library test functions

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include cctest program header files

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRun.h"
#include "ccRef.h"

/*---------------------------------------------------------------------------------------------------------*/
enum fg_gen_status ccRefDirectGen(struct fg_table *pars, const double *time, float *ref)
/*---------------------------------------------------------------------------------------------------------*/
{
    double                       func_time;              // Time since end of run delay
    float                        prev_rate;
    static float                 prev_ref;
    static float                 next_ref;
    static float                 final_ref;
    static enum fg_gen_status    fg_gen_status;

    // Coast during run delay

    func_time = *time - pars->delay;

    if(func_time < 0.0)
    {
        return(FG_GEN_BEFORE_FUNC);
    }

    // If DIRECT function is already running

    if(pars->seg_idx > 0)
    {
        prev_rate = (*ref - prev_ref) / conv.reg_period;
        prev_ref  = *ref;
    }
    else // DIRECT has not yet started
    {
        // Prepare to force initialisation of first RAMP function

        fg_gen_status = FG_GEN_AFTER_FUNC;
        prev_rate     = 0.0;
        prev_ref      = *ref;
        final_ref     = 1.0E30;
    }

    // Scan through table to find segment containing the current time

    while(func_time >= pars->time[pars->seg_idx])      // while time exceeds end of segment
    {
        // If vector is already complete or is now complete

        if(pars->seg_idx >= pars->num_points || ++pars->seg_idx >= pars->num_points)
        {
            // Return function running flag from RAMP function

            return(fg_gen_status);
        }
    }

    // If target reference has changed then re-arm the RAMP function

    if(pars->ref[pars->seg_idx] != next_ref)
    {
        // Initialise RAMP based on reg_mode

        final_ref = next_ref = pars->ref[pars->seg_idx];

        // Clip reference

        if(conv.reg_signal->lim_ref.invert_limits == REG_DISABLED)
        {
            if(next_ref > conv.lim_ref->pos)
            {
                final_ref = conv.lim_ref->pos;
            }
            else if(next_ref < conv.lim_ref->neg)
            {
                final_ref = conv.lim_ref->neg;
            }
        }
        else
        {
            if(next_ref > -conv.lim_ref->neg)
            {
                final_ref = -conv.lim_ref->neg;
            }
            else if(next_ref < -conv.lim_ref->pos)
            {
                final_ref = -conv.lim_ref->pos;
            }
        }

        // Initialise RAMP to new reference

        fgRampCalc(ccpars_load.pol_swi_auto,
                   ccpars_limits.invert, 
                   func_time - conv.reg_period,
                   prev_rate,
                   *ref,
                   final_ref,
                   ccpars_default.pars[conv.reg_mode].acceleration,
                   ccpars_default.pars[conv.reg_mode].linear_rate,
                   ccpars_default.pars[conv.reg_mode].deceleration,
                   &ccrun.prefunc.pars,
                   NULL);
    }

    fg_gen_status = fgRampGen(&ccrun.prefunc.pars, &func_time, ref);

    return(FG_GEN_DURING_FUNC);
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccRefInitPLEP(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    return(fgPlepInit(  ccrun.fg_limits,
                        ccpars_load.pol_swi_auto,
                        ccpars_limits.invert, 
                        ccpars_global.run_delay,
                        ccpars_plep[cyc_sel].initial_ref,
                        ccpars_plep[cyc_sel].final_ref,
                        ccpars_plep[cyc_sel].final_rate,
                        ccpars_plep[cyc_sel].acceleration,
                        ccpars_plep[cyc_sel].linear_rate,
                        ccpars_plep[cyc_sel].exp_tc,
                        ccpars_plep[cyc_sel].exp_final,
                        &fg_plep[cyc_sel],
                        fg_meta));
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccRefInitRAMP(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    return(fgRampInit(  ccrun.fg_limits,
                        ccpars_load.pol_swi_auto,
                        ccpars_limits.invert, 
                        ccpars_global.run_delay,
                        ccpars_ramp[cyc_sel].initial_ref,
                        ccpars_ramp[cyc_sel].final_ref,
                        ccpars_ramp[cyc_sel].acceleration,
                        ccpars_ramp[cyc_sel].linear_rate,
                        ccpars_ramp[cyc_sel].deceleration,
                        &fg_ramp[cyc_sel],
                        fg_meta));
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccRefInitPPPL(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    return(fgPpplInit(  ccrun.fg_limits,
                        ccpars_load.pol_swi_auto,
                        ccpars_limits.invert, 
                        ccpars_global.run_delay,
                        ccpars_pppl[cyc_sel].initial_ref,
                        ccpars_pppl[cyc_sel].acceleration1,
                        pppl_pars[1].num_elements[cyc_sel],
                        ccpars_pppl[cyc_sel].acceleration2,
                        pppl_pars[2].num_elements[cyc_sel],
                        ccpars_pppl[cyc_sel].acceleration3,
                        pppl_pars[3].num_elements[cyc_sel],
                        ccpars_pppl[cyc_sel].rate2,
                        pppl_pars[4].num_elements[cyc_sel],
                        ccpars_pppl[cyc_sel].rate4,
                        pppl_pars[5].num_elements[cyc_sel],
                        ccpars_pppl[cyc_sel].ref4,
                        pppl_pars[6].num_elements[cyc_sel],
                        ccpars_pppl[cyc_sel].duration4,
                        pppl_pars[7].num_elements[cyc_sel],
                        &fg_pppl[cyc_sel],
                        fg_meta));
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccRefInitTABLE(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    return(fgTableInit( ccrun.fg_limits,
                        ccpars_load.pol_swi_auto,
                        ccpars_limits.invert, 
                        ccpars_global.run_delay,
                        conv.iter_period,
                        ccpars_table[cyc_sel].ref,
                        table_pars[0].num_elements[cyc_sel],
                        ccpars_table[cyc_sel].time,
                        table_pars[1].num_elements[cyc_sel],
                        &fg_table[cyc_sel],
                        fg_meta));
    return(EXIT_SUCCESS);
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccRefInitSTEPS(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    return(fgTestInit(  ccrun.fg_limits,
                        ccpars_load.pol_swi_auto,
                        ccpars_limits.invert, 
                        ccpars_global.run_delay,
                        FG_TEST_STEPS,
                        ccpars_test[cyc_sel].initial_ref,
                        ccpars_test[cyc_sel].amplitude_pp,
                        ccpars_test[cyc_sel].num_cycles,
                        ccpars_test[cyc_sel].period,
                        false,                                  // is_window_active not used for STEPS
                        &fg_test[cyc_sel],
                        fg_meta));
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccRefInitSQUARE(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    return(fgTestInit(  ccrun.fg_limits,
                        ccpars_load.pol_swi_auto,
                        ccpars_limits.invert, 
                        ccpars_global.run_delay,
                        FG_TEST_SQUARE,
                        ccpars_test[cyc_sel].initial_ref,
                        ccpars_test[cyc_sel].amplitude_pp,
                        ccpars_test[cyc_sel].num_cycles,
                        ccpars_test[cyc_sel].period,
                        false,                                  // is_window_active not used for SQUARE
                        &fg_test[cyc_sel],
                        fg_meta));
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccRefInitSINE(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    return(fgTestInit(  ccrun.fg_limits,
                        ccpars_load.pol_swi_auto,
                        ccpars_limits.invert, 
                        ccpars_global.run_delay,
                        FG_TEST_SINE,
                        ccpars_test[cyc_sel].initial_ref,
                        ccpars_test[cyc_sel].amplitude_pp,
                        ccpars_test[cyc_sel].num_cycles,
                        ccpars_test[cyc_sel].period,
                        ccpars_test[cyc_sel].use_window,
                        &fg_test[cyc_sel],
                        fg_meta));
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccRefInitCOSINE(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    return(fgTestInit(  ccrun.fg_limits,
                        ccpars_load.pol_swi_auto,
                        ccpars_limits.invert, 
                        ccpars_global.run_delay,
                        FG_TEST_COSINE,
                        ccpars_test[cyc_sel].initial_ref,
                        ccpars_test[cyc_sel].amplitude_pp,
                        ccpars_test[cyc_sel].num_cycles,
                        ccpars_test[cyc_sel].period,
                        ccpars_test[cyc_sel].use_window,
                        &fg_test[cyc_sel],
                        fg_meta));
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccRefInitLTRIM(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    return(fgTrimInit(  ccrun.fg_limits,
                        ccpars_load.pol_swi_auto,
                        ccpars_limits.invert, 
                        ccpars_global.run_delay,
                        FG_TRIM_LINEAR,
                        ccpars_trim[cyc_sel].initial_ref,
                        ccpars_trim[cyc_sel].final_ref,
                        ccpars_trim[cyc_sel].duration,
                        &fg_trim[cyc_sel],
                        fg_meta));
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccRefInitCTRIM(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    return(fgTrimInit(  ccrun.fg_limits,
                        ccpars_load.pol_swi_auto,
                        ccpars_limits.invert, 
                        ccpars_global.run_delay,
                        FG_TRIM_CUBIC,
                        ccpars_trim[cyc_sel].initial_ref,
                        ccpars_trim[cyc_sel].final_ref,
                        ccpars_trim[cyc_sel].duration,
                        &fg_trim[cyc_sel],
                        fg_meta));
}
/*---------------------------------------------------------------------------------------------------------*/
enum fg_error ccRefInitPULSE(struct fg_meta *fg_meta, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    // Initialise a flat TRIM to produce the flat reference of the required duration, at the required time

    return(fgTrimInit(  ccrun.fg_limits,
                        ccpars_load.pol_swi_auto,
                        ccpars_limits.invert, 
                        ccpars_global.run_delay + ccpars_pulse[cyc_sel].time,
                        FG_TRIM_LINEAR,
                        ccpars_pulse[cyc_sel].ref,
                        ccpars_pulse[cyc_sel].ref,
                        ccpars_pulse[cyc_sel].duration,
                        &fg_pulse[cyc_sel],
                        fg_meta));
}

// EOF

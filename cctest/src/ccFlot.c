/*---------------------------------------------------------------------------------------------------------*\
  File:     ccFlot.c                                                                    Copyright CERN 2014

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

  Purpose:  cctest program flot chart web page generation cycletions
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRun.h"
#include "ccSigs.h"
#include "ccFlot.h"
#include "ccDebug.h"
#include "flot.h"



static uint32_t ccFlotRefs(FILE *f, double end_time)
{
    uint32_t       cyc_sel;
    uint32_t       num_points;

    // For each cycle selector

    for(cyc_sel = num_points = 0 ; cyc_sel < CC_NUM_CYC_SELS ; cyc_sel++)
    {
        if(ccrun.is_used[cyc_sel])
        {
            uint32_t cycle_idx;

            fprintf(f,"\"(%u) %s\": { lines: { show:false }, points: { show:true }, downsample: { threshold: 0 },\ndata:[",
                      cyc_sel, ccParsEnumString(enum_function_type, ccpars_ref[cyc_sel].function));

            for(cycle_idx = 0 ; cycle_idx < ccrun.num_cycles ; cycle_idx++)
            {
                if(cyc_sel == ccrun.cycle[cycle_idx].cyc_sel)
                {
                    uint32_t    n;
                    uint32_t    iteration_idx;
                    double      time = 0.0;;
                    double      end_cycle_time;

                    fprintf(f,"[%.6f,%.7E],[%.6f,%.7E],",
                              ccrun.cycle[cycle_idx].start_time,
                              ccrun.fg_meta[cyc_sel].range.start,
                              ccrun.cycle[cycle_idx].start_time + ccpars_global.run_delay,
                              ccrun.fg_meta[cyc_sel].range.start);

                    num_points += 3;

                    switch(ccpars_ref[cyc_sel].function)
                    {
                    default: break;     // Suppress compiler warning

                    case FG_TABLE:
                    case FG_DIRECT:

                        n = table_pars[0].num_elements[cyc_sel] - 1;

                        for(iteration_idx = 1 ; iteration_idx < n ; iteration_idx++)
                        {
                            time = ccrun.cycle[cycle_idx].start_time + ccpars_global.run_delay + ccpars_table[cyc_sel].time[iteration_idx];

                            if(time < end_time)
                            {
                                fprintf(f,"[%.6f,%.7E],", time, ccpars_table[cyc_sel].ref[iteration_idx]);
                                num_points++;
                            }
                        }
                        break;

                    case FG_PPPL:

                        time = ccrun.cycle[cycle_idx].start_time + ccpars_global.run_delay;

                        fprintf(f,"[%.6f,%.7E],", time, ccpars_pppl[cyc_sel].initial_ref);
                        num_points++;

                        n = fg_pppl[cyc_sel].num_segs - 1;

                        for(iteration_idx = 1 ; iteration_idx < n ; iteration_idx++)
                        {
                            time = ccrun.cycle[cycle_idx].start_time + ccpars_global.run_delay + fg_pppl[cyc_sel].time[iteration_idx];

                            if(time < end_time)
                            {
                                fprintf(f,"[%.6f,%.7E],", time, fg_pppl[cyc_sel].a0[iteration_idx]);
                                num_points++;
                            }
                        }
                        break;

                    case FG_PLEP:

                        time = ccrun.cycle[cycle_idx].start_time + ccpars_global.run_delay;

                        fprintf(f,"[%.6f,%.7E],", time, ccpars_plep[cyc_sel].initial_ref);
                        num_points++;

                        for(iteration_idx = 1 ; iteration_idx <  FG_PLEP_NUM_SEGS ; iteration_idx++)
                        {
                            time = ccrun.cycle[cycle_idx].start_time + ccpars_global.run_delay + fg_plep[cyc_sel].time[iteration_idx];

                            if(time < end_time)
                            {
                                fprintf(f,"[%.6f,%.7E],", time, fg_plep[cyc_sel].normalisation * fg_plep[cyc_sel].ref[iteration_idx]);
                                num_points++;
                            }
                        }
                        break;
                    }

                    // End of function point

                    end_cycle_time = ccrun.cycle[cycle_idx].start_time + ccpars_global.run_delay + ccrun.fg_meta[cyc_sel].duration;

                    if(end_cycle_time > time && end_cycle_time < end_time)
                    {
                        fprintf(f,"[%.6f,%.7E],", end_cycle_time, ccrun.fg_meta[cyc_sel].range.end);
                    }
                }
            }
            fputs("]\n },\n",f);
        }
    }

    return(num_points);
}



static uint32_t ccFlotDynEco(FILE *f, double end_time)
{
    uint32_t       num_points = 0;

    if(ccrun.dyn_eco.log.length > 0 && ccrun.dyn_eco.log.time[0] < end_time)
    {
        uint32_t       sig_idx;

        fputs("\"DYN_ECO\": { lines: { show:false }, points: { show:true },\ndata:[",f);

        for(sig_idx = 0 ; sig_idx < ccrun.dyn_eco.log.length && ccrun.dyn_eco.log.time[sig_idx] < end_time ; sig_idx++, num_points++)
        {
            fprintf(f,"[%.6f,%.7E],", ccrun.dyn_eco.log.time[sig_idx], ccrun.dyn_eco.log.ref[sig_idx]);
        }
        fputs("]\n },\n",f);
    }

    return(num_points);
}



static uint32_t ccFlotAnalog(FILE *f)
{
    uint32_t       sig_idx;
    uint32_t       num_points;

    // Print enabled analog signal values

    for(sig_idx = num_points = 0 ; sig_idx < NUM_SIGNALS ; sig_idx++)
    {
        if(signals[sig_idx].control == REG_ENABLED && signals[sig_idx].type == ANALOG)
        {
            uint32_t       iteration_idx;
            float          time_offset;

            time_offset = signals[sig_idx].time_offset;

            fprintf(f,"\"%s\": { lines: { steps:%s }, points: { show:false }, %s\ndata:[",
                    signals[sig_idx].name,
                    signals[sig_idx].meta_data[0] == 'T' ? "true" : "false",
                    signals[sig_idx].meta_data[0] == 'T' ? "downsample: { threshold: 0 }," : "");


            for(iteration_idx = 0; iteration_idx < flot_index; iteration_idx++)
            {
                // Only print changed values when meta_data is TRAIL_STEP

                if(iteration_idx == 0 ||
                   iteration_idx >= (flot_index - 1) ||
                   signals[sig_idx].meta_data[0] != 'T' ||
                   signals[sig_idx].buf[iteration_idx] != signals[sig_idx].buf[iteration_idx-1])
                {
                    double  time;

                    if(ccpars_global.reverse_time == REG_DISABLED)
                    {
                        time = conv.iter_period * iteration_idx + time_offset;
                    }
                    else
                    {
                        time = conv.iter_period * (ccrun.num_iterations - iteration_idx - 1);
                    }

                    fprintf(f,"[%.6f,%.7E],", time, signals[sig_idx].buf[iteration_idx]);
                    num_points++;
                }
            }
            fputs("]\n },\n",f);
        }
    }

    return(num_points);
}



static uint32_t ccFlotDigital(FILE *f)
{
    uint32_t       sig_idx;
    uint32_t       num_points;
    float          dig_offset;

    for(sig_idx = num_points = 0, dig_offset = -DIG_STEP/2.0 ; sig_idx < NUM_SIGNALS ; sig_idx++)
    {
        if(signals[sig_idx].control == REG_ENABLED && signals[sig_idx].type == DIGITAL)
        {
            uint32_t  iteration_idx;

            dig_offset -= 1.0;

            fprintf(f,"\"%s\": {\n lines: { steps:%s },\n downsample: { threshold: 0 },\n data:[",
                    signals[sig_idx].name,
                    signals[sig_idx].meta_data[0] == 'T' ? "true" : "false");

            for(iteration_idx = 0; iteration_idx < flot_index; iteration_idx++)
            {
                double time;

                // Only print changed values when meta_data is TRAIL_STEP

                if(iteration_idx == 0 ||
                   iteration_idx == (flot_index - 1) ||
                   signals[sig_idx].meta_data[0] != 'T' ||
                   signals[sig_idx].buf[iteration_idx] != signals[sig_idx].buf[iteration_idx-1])
                {
                    if(ccpars_global.reverse_time == REG_DISABLED)
                    {
                        time = conv.iter_period * iteration_idx;
                    }
                    else
                    {
                        time = conv.iter_period * (ccrun.num_iterations - iteration_idx - 1);
                    }

                    fprintf(f,"[%.6f,%.2f],", time, signals[sig_idx].buf[iteration_idx] + dig_offset);
                    num_points++;
                }
            }
            fputs("]\n },\n",f);
        }
    }

    return(num_points);
}



void ccFlot(FILE *f, char *filename)
{;
    uint32_t       num_points;
    struct cccmds *cmd;
    double         end_time = (double)flot_index * 1.0E-6 * (double)ccpars_global.iter_period_us;

    // Warn user if FLOT data was truncated

    if(flot_index >= ccpars_global.flot_points_max)
    {
        printf("Warning - FLOT data truncated to %u points\n",ccpars_global.flot_points_max);
    }

    // Print start of FLOT html page including flot path to all the javascript libraries

    fprintf(f,flot[0],filename,FLOT_PATH,FLOT_PATH,FLOT_PATH,FLOT_PATH,FLOT_PATH,FLOT_PATH,FLOT_PATH);

    // Create Flot signals using points to represent the reference data

    num_points = ccFlotRefs(f, end_time);

    // Create a Flot signal to mark dynamic economy, if in use

    num_points += ccFlotDynEco(f, end_time);

    // Print enabled analog signal values

    num_points += ccFlotAnalog(f);

    // Print start of digital signals

    fputs(flot[1],f);

    // Print enabled digital signal values

    num_points += ccFlotDigital(f);

    // Print command parameter values to become a colorbox pop-up

    fprintf(f, flot[2], CC_VERSION);    // Version is embedded in the About pop-up title: "About cctest vx.xx"

    for(cmd = cmds ; cmd->name != NULL ; cmd++)
    {
        if(cmd->is_enabled == true)
        {
            fputc('\n',f);
            ccParsPrintAll(f, cmd->name, cmd->pars, CC_ALL_CYCLES, CC_NO_INDEX);
        }
    }

    // Print debug variable to become a colorbox pop-up

    fputs(flot[3],f);

    fprintf(f,"%-*s  %u\n\n", PARS_INDENT, "FLOT num_points", num_points);

    ccDebugPrint(f);

    // Write HTML file footer

    fputs(flot[4],f);
}

// EOF

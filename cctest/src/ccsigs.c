/*---------------------------------------------------------------------------------------------------------*\
  File:     ccsigs.c                                                                    Copyright CERN 2011

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

  Purpose:  fg and reg library test program signal functions

  Authors:  Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "ccpars.h"
#include "ccsigs.h"
#include "flot.h"

#define DIG_STEP        0.5      // Digital signal step size

static unsigned flot_index;      // Index into flot buffers
static float    dig_offset;      // Offset to stack digital signals for FGCSPY and LVDV output formats

/*---------------------------------------------------------------------------------------------------------*/
static void ccsigsEnableSignal(enum ccsig_idx idx)
/*---------------------------------------------------------------------------------------------------------*\
  This function will enable a signal and define its type to be ANALOG, DIGITAL or CURSOR. If the
  output format is FGCSPY or LVDV then for each new digital signal the offset is moved down by -1.0
  so that they do not overlap on the graphing tool when looking at the results.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Enable signal

    signals[idx].flag = CC_ENABLED;

    // Set offset for digital signals for FGCSPY and LVDV output formats

    if(signals[idx].type == DIGITAL &&
       (ccpars_global.output_format == CC_FGCSPY ||
        ccpars_global.output_format == CC_LVDV))
    {
        dig_offset -= 1.0;

        signals[idx].dig_offset = dig_offset;
    }

    // If FLOT output enabled then allocate buffer memory for non-Cursor signals

    if(ccpars_global.output_format == CC_FLOT && signals[idx].type != CURSOR)
    {
        signals[idx].buf = (float *)calloc(MAX_FLOT_POINTS+1, sizeof(double));  // On Mac, floats are doubles
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccsigsStoreAnalog(enum ccsig_idx idx, float ana_value)
/*---------------------------------------------------------------------------------------------------------*\
  This function will store an analog signal value that was previously enabled by ccsigsEnableSignal().
\*---------------------------------------------------------------------------------------------------------*/
{
    if(signals[idx].type != ANALOG)
    {
        fprintf(stderr,"Error: Attempt to store an analog value in signal %s which is not enabled as ANALOG\n",
                        signals[idx].name);
        exit(EXIT_FAILURE);
    }

    // Store analogue value

    signals[idx].value = ana_value;

    // If FLOT output format enabled then also save value in the FLOT buffer

    if(ccpars_global.output_format == CC_FLOT)
    {
        signals[idx].buf[flot_index] = ana_value;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccsigsStoreDigital(enum ccsig_idx idx, uint32_t dig_value)
/*---------------------------------------------------------------------------------------------------------*\
  This function will store a digital signal value that was previously enabled by ccsigsEnableSignal().
\*---------------------------------------------------------------------------------------------------------*/
{
    if(signals[idx].type != DIGITAL)
    {
        fprintf(stderr,"Error: Attempt to store a digital value in signal %s which is not enabled as DIGITAL\n",
                        signals[idx].name);
        exit(EXIT_FAILURE);
    }

    // Store digital level as a float using the digital offset

    signals[idx].value = signals[idx].dig_offset;       // Initialise value to digital zero

    if(dig_value != 0)
    {
        // For digital one, the step depends on the output format

        if(ccpars_global.output_format == CC_STANDARD)
        {
            signals[idx].value += 1.0;
        }
        else
        {
            signals[idx].value += DIG_STEP;
        }
    }

    // If FLOT output format enabled then also save value in the FLOT buffer

    if(ccpars_global.output_format == CC_FLOT)
    {
        signals[idx].buf[flot_index] = signals[idx].value;
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccsigsStoreCursor(enum ccsig_idx idx, char *cursor_label)
/*---------------------------------------------------------------------------------------------------------*\
  This function will store a cursor signal value that was previously enabled by ccsigsEnableSignal().
\*---------------------------------------------------------------------------------------------------------*/
{
    if(signals[idx].type != CURSOR)
    {
        fprintf(stderr,"Error: Attempt to store a cursor value in signal %s which is not enabled as CURSOR\n",
                        signals[idx].name);
        exit(EXIT_FAILURE);
    }

    // Store the label

    signals[idx].cursor_label = cursor_label;
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccsigsPrintHeader(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print the signal headers to stdout for non-FLOT output formats
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t idx;

    // If not FLOT output format

    if(ccpars_global.output_format != CC_FLOT)
    {
        // First row: print enabled signal headers
        // Add _D suffix for digital signals if output is for FGCSPY

        fputs("TIME",stdout);

        for(idx = 0 ; idx < NUM_SIGNALS ; idx++)
        {
            if(signals[idx].flag == CC_ENABLED)
            {
                printf( ",%s%s", signals[idx].name,
                        ccpars_global.output_format == CC_FGCSPY &&
                        signals[idx].meta_data[0] == 'T' ? "_D" : "");
            }
        }

        // Second row: if ouput is for the Labview Dataviewer (LVDV) then add meta data line

        if(ccpars_global.output_format == CC_LVDV)
        {
            fputs("\nMETA",stdout);

            for(idx = 0 ; idx < NUM_SIGNALS ; idx++)
            {
                if(signals[idx].flag == CC_ENABLED)
                {
                    printf(",%s",signals[idx].meta_data);
                }
            }
        }

        fputc('\n',stdout);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccsigsPrintValues(float time)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print the signal values to stdout.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t   idx;

    // If FLOT output enabled then print flot header

    if(ccpars_global.output_format == CC_FLOT)
    {
        if(flot_index < MAX_FLOT_POINTS)
        {
            flot_index++;
        }
    }
    else
    {
        // Print the timestamp first with microsecond resolution

        printf("%.6f",time);

        // Print enabled signal values

        for(idx = 0 ; idx < NUM_SIGNALS ; idx++)
        {
            if(signals[idx].flag == CC_ENABLED)
            {
                putchar(',');

                switch(signals[idx].type)
                {
                case ANALOG:

                    printf("%.7E",signals[idx].value);
                    break;

                case DIGITAL:

                    printf("%.1f",signals[idx].value);
                    break;

                case CURSOR:        // Cursor values - clear cursor label after printing

                    if(signals[idx].cursor_label != NULL)
                    {
                        printf("%s",signals[idx].cursor_label);
                        signals[idx].cursor_label = NULL;
                    }
                    break;
                }
            }
        }

        fputc('\n',stdout);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccsigsPrepare(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will enable the signals that need to be stored according to the mode of the run.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Voltage reference is always enabled

    ccsigsEnableSignal(ANA_V_REF);

    // Enable additional signals with simulating load

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        // Enable cursor signals only if output is for the Labview Dataviewer (LVDV)

        if(ccpars_global.output_format == CC_LVDV)
        {
            ccsigsEnableSignal(CSR_LOAD);
            ccsigsEnableSignal(CSR_REGMODE);
            ccsigsEnableSignal(CSR_REF);
        }

        // Voltage source simulation signals

        ccsigsEnableSignal(ANA_V_REF_LIMITED);
        ccsigsEnableSignal(ANA_V_MEAS);
        ccsigsEnableSignal(ANA_V_ERR);
        ccsigsEnableSignal(ANA_MAX_ABS_V_ERR);
        ccsigsEnableSignal(DIG_V_REF_CLIP);
        ccsigsEnableSignal(DIG_V_REF_RATE_CLIP);
        ccsigsEnableSignal(DIG_V_REG_ERR_WARN);
        ccsigsEnableSignal(DIG_V_REG_ERR_FLT);

        switch(ccpars_global.units)
        {
        case REG_FIELD:

            // Field regulation signals

            ccsigsEnableSignal(ANA_B_REF);
            ccsigsEnableSignal(ANA_B_REF_LIMITED);
            ccsigsEnableSignal(ANA_B_REF_RST);
            ccsigsEnableSignal(ANA_B_MEAS);
            ccsigsEnableSignal(ANA_B_MEAS_FLTR);
            ccsigsEnableSignal(ANA_B_REG);
            ccsigsEnableSignal(ANA_B_ERR);
            ccsigsEnableSignal(ANA_MAX_ABS_B_ERR);
            ccsigsEnableSignal(DIG_B_MEAS_TRIP);
            ccsigsEnableSignal(DIG_B_MEAS_LOW);
            ccsigsEnableSignal(DIG_B_MEAS_ZERO);
            ccsigsEnableSignal(DIG_B_REF_CLIP);
            ccsigsEnableSignal(DIG_B_REF_RATE_CLIP);
            ccsigsEnableSignal(DIG_B_REG_ERR_WARN);
            ccsigsEnableSignal(DIG_B_REG_ERR_FLT);
            break;

        case REG_CURRENT:

            // Current regulation signals

            ccsigsEnableSignal(ANA_I_REG);
            ccsigsEnableSignal(ANA_I_REF);
            ccsigsEnableSignal(ANA_I_REF_LIMITED);
            ccsigsEnableSignal(ANA_I_REF_RST);
            ccsigsEnableSignal(ANA_I_ERR);
            ccsigsEnableSignal(ANA_MAX_ABS_I_ERR);
            ccsigsEnableSignal(ANA_V_REF_SAT);
            ccsigsEnableSignal(DIG_I_REF_CLIP);
            ccsigsEnableSignal(DIG_I_REF_RATE_CLIP);
            ccsigsEnableSignal(DIG_I_REG_ERR_WARN);
            ccsigsEnableSignal(DIG_I_REG_ERR_FLT);
            break;
        }

        // Current simulation signals

        ccsigsEnableSignal(ANA_I_MEAS);
        ccsigsEnableSignal(ANA_I_MEAS_FLTR);
        ccsigsEnableSignal(DIG_I_MEAS_TRIP);
        ccsigsEnableSignal(DIG_I_MEAS_LOW);
        ccsigsEnableSignal(DIG_I_MEAS_ZERO);
    }

    // Write signal headers

    ccsigsPrintHeader();
}
/*---------------------------------------------------------------------------------------------------------*/
void ccsigsStore(float time)
/*---------------------------------------------------------------------------------------------------------*\
  This function will store all the signals for the current iteration.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Voltage reference is always stored

    ccsigsStoreAnalog(ANA_V_REF, reg.v_ref);

    // Store other signals when simulating load

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        // Store voltage reference signals

        ccsigsStoreAnalog (ANA_V_MEAS,         reg.v_meas.unfiltered);
        ccsigsStoreAnalog (ANA_V_REF,          reg.v_ref);
        ccsigsStoreAnalog (ANA_V_REF_LIMITED,  reg.v_ref_limited);
        ccsigsStoreAnalog (ANA_V_ERR,          reg.v_err.err);
        ccsigsStoreAnalog (ANA_MAX_ABS_V_ERR,  reg.v_err.max_abs_err);

        ccsigsStoreDigital(DIG_V_REG_ERR_FLT,  reg.v_err.fault.flag);
        ccsigsStoreDigital(DIG_V_REG_ERR_WARN, reg.v_err.warning.flag);
        ccsigsStoreDigital(DIG_V_REF_CLIP,     reg.lim_v_ref.flags.clip);
        ccsigsStoreDigital(DIG_V_REF_RATE_CLIP,reg.lim_v_ref.flags.rate);

        // Store field or current signals according to regulation units

        switch(ccpars_global.units)
        {
        case REG_FIELD:

            // Field regulation signals

            ccsigsStoreAnalog( ANA_B_MEAS,         reg.b_meas.unfiltered);
            ccsigsStoreAnalog( ANA_B_MEAS_FLTR,    reg.b_meas.filtered);
            ccsigsStoreAnalog( ANA_B_REG,          reg.b_meas.regulated);
            ccsigsStoreAnalog (ANA_B_REF,          reg.ref);
            ccsigsStoreAnalog (ANA_B_REF_LIMITED,  reg.ref_limited);
            ccsigsStoreAnalog (ANA_B_REF_RST,      reg.ref_rst);
            ccsigsStoreAnalog (ANA_B_ERR,          reg.err);
            ccsigsStoreAnalog (ANA_MAX_ABS_B_ERR,  reg.max_abs_err);

            ccsigsStoreDigital(DIG_B_MEAS_TRIP,    reg.lim_b_meas.flags.trip);
            ccsigsStoreDigital(DIG_B_MEAS_LOW,     reg.lim_b_meas.flags.low);
            ccsigsStoreDigital(DIG_B_MEAS_ZERO,    reg.lim_b_meas.flags.zero);
            ccsigsStoreDigital(DIG_B_REF_CLIP,     reg.lim_b_ref.flags.clip);
            ccsigsStoreDigital(DIG_B_REF_RATE_CLIP,reg.lim_b_ref.flags.rate);
            ccsigsStoreDigital(DIG_B_REG_ERR_FLT,  reg.b_err.limits.fault.flag);
            ccsigsStoreDigital(DIG_B_REG_ERR_WARN, reg.b_err.limits.warning.flag);
            break;

        case REG_CURRENT:

            // Current regulation signals

            ccsigsStoreAnalog (ANA_I_REG,          reg.i_meas.regulated);
            ccsigsStoreAnalog (ANA_I_REF,          reg.ref);
            ccsigsStoreAnalog (ANA_I_REF_LIMITED,  reg.ref_limited);
            ccsigsStoreAnalog (ANA_I_REF_RST,      reg.ref_rst);
            ccsigsStoreAnalog (ANA_I_ERR,          reg.err);
            ccsigsStoreAnalog (ANA_V_REF_SAT,      reg.v_ref_sat);
            ccsigsStoreAnalog (ANA_MAX_ABS_I_ERR,  reg.max_abs_err);

            ccsigsStoreDigital(DIG_I_REF_CLIP,     reg.lim_i_ref.flags.clip);
            ccsigsStoreDigital(DIG_I_REF_RATE_CLIP,reg.lim_i_ref.flags.rate);
            ccsigsStoreDigital(DIG_I_REG_ERR_FLT,  reg.i_err.limits.fault.flag);
            ccsigsStoreDigital(DIG_I_REG_ERR_WARN, reg.i_err.limits.warning.flag);
            break;
        }

        // Store current simulation signals

        ccsigsStoreAnalog (ANA_I_MEAS,      reg.i_meas.unfiltered);
        ccsigsStoreAnalog (ANA_I_MEAS_FLTR, reg.i_meas.filtered);

        ccsigsStoreDigital(DIG_I_MEAS_TRIP, reg.lim_i_meas.flags.trip);
        ccsigsStoreDigital(DIG_I_MEAS_LOW,  reg.lim_i_meas.flags.low);
        ccsigsStoreDigital(DIG_I_MEAS_ZERO, reg.lim_i_meas.flags.zero);
    }

    // Print enabled values to stdout

    ccsigsPrintValues(time);
}
/*---------------------------------------------------------------------------------------------------------*/
void ccsigsFlot(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print the flot data and footer
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t   iteration_idx;
    uint32_t   sig_idx;
    double     time;
    double     end_time;

    // If FLOT output enabled then print flot header

    if(ccpars_global.output_format == CC_FLOT)
    {
        // Print start of FLOT html page including flot path to all the javascript libraries

        printf(flot[0],
               ccpars_global.flot_path,
               ccpars_global.flot_path,
               ccpars_global.flot_path,
               ccpars_global.flot_path,
               ccpars_global.flot_path,
               ccpars_global.flot_path);

        // Print table, pppl or plep data if selected and use points instead of lines

        if(ccpars_global.function == FG_TABLE ||
           ccpars_global.function == FG_PPPL ||
           ccpars_global.function == FG_PLEP)
        {
            printf("\"%s\": { lines: { show:false }, points: { show:true },\ndata:[",
                   ccparsEnumString(function_type, ccpars_global.function));

            end_time = flot_index * reg.iter_period;

            switch(ccpars_global.function)
            {
            default: break;     // Suppress gcc warning

            case FG_TABLE:

                for(iteration_idx = 0 ; iteration_idx < table_pars_list[0].num_values &&
                                        (time = ccpars_table.time[iteration_idx] + ccpars_global.run_delay) < end_time ; iteration_idx++)
                {
                    printf("[%.6f,%.7E],", time, ccpars_table.ref[iteration_idx]);
                }
                break;

            case FG_PPPL:

                printf("[%.6f,%.7E],", ccpars_global.run_delay, ccpars_pppl.initial_ref);

                for(iteration_idx = 0 ; iteration_idx < ccpars_pppl.pppl_pars.num_segs &&
                                        ccpars_pppl.pppl_pars.time[iteration_idx] < end_time ; iteration_idx++)
                {
                    printf("[%.6f,%.7E],",
                            ccpars_pppl.pppl_pars.time[iteration_idx],
                            ccpars_pppl.pppl_pars.a0  [iteration_idx]);
                }
                break;

            case FG_PLEP:

                printf("[%.6f,%.7E],", ccpars_global.run_delay, ccpars_plep.initial_ref);

                for(iteration_idx = 0 ; iteration_idx <= FG_PLEP_N_SEGS && 
                                        ccpars_plep.plep_pars.time[iteration_idx] < end_time ; iteration_idx++)
                {
                    printf("[%.6f,%.7E],",
                            ccpars_plep.plep_pars.time[iteration_idx],
                            ccpars_plep.plep_pars.normalisation * ccpars_plep.plep_pars.ref[iteration_idx]);
                }
                break;
            }

            puts("]\n },");
        }

        // Print enabled analogue signal values

        for(sig_idx = 0 ; sig_idx < NUM_SIGNALS ; sig_idx++)
        {
            if(signals[sig_idx].flag == CC_ENABLED && signals[sig_idx].type == ANALOG)
            {
                printf("\"%s\": { lines: { steps:%s }, points: { show:false },\ndata:[",
                        signals[sig_idx].name,
                        signals[sig_idx].meta_data[0] == 'T' ? "true" : "false");

                for(iteration_idx = 0; iteration_idx < flot_index; iteration_idx++)
                {
                    // Only print changed values when meta_data is TRAIL_STEP

                    if(iteration_idx == 0 ||
                       iteration_idx == (flot_index - 1) ||
                       signals[sig_idx].meta_data[0] != 'T' ||
                       signals[sig_idx].buf[iteration_idx] != signals[sig_idx].buf[iteration_idx-1])
                    {
                        if(ccpars_global.reverse_time == CC_DISABLED)
                        {
                            time = reg.iter_period * iteration_idx;
                        }
                        else
                        {
                            time = reg.iter_period * (ccpars_global.num_iterations - iteration_idx - 1);
                        }

                        printf("[%.6f,%.7E],", time, signals[sig_idx].buf[iteration_idx]);
                    }
                }
                puts("]\n },");
            }
        }

        // Print start of digital signals

        puts(flot[1]);

        // Print enabled digital signal values

        for(sig_idx = 0, dig_offset = -DIG_STEP/2.0 ; sig_idx < NUM_SIGNALS ; sig_idx++)
        {
            if(signals[sig_idx].flag == CC_ENABLED && signals[sig_idx].type == DIGITAL)
            {
                dig_offset -= 1.0;

                printf("\"%s\": {\n lines: { steps:%s },\n data:[",
                        signals[sig_idx].name,
                        signals[sig_idx].meta_data[0] == 'T' ? "true" : "false");

                for(iteration_idx = 0; iteration_idx < flot_index; iteration_idx++)
                {
                    // Only print changed values when meta_data is TRAIL_STEP

                    if(iteration_idx == 0 ||
                       iteration_idx == (flot_index - 1) ||
                       signals[sig_idx].meta_data[0] != 'T' ||
                       signals[sig_idx].buf[iteration_idx] != signals[sig_idx].buf[iteration_idx-1])
                    {
                        if(ccpars_global.reverse_time == CC_DISABLED)
                        {
                            time = reg.iter_period * iteration_idx;
                        }
                        else
                        {
                            time = reg.iter_period * (ccpars_global.num_iterations - iteration_idx - 1);
                        }

                        printf("[%.6f,%.2f],", time, signals[sig_idx].buf[iteration_idx] + dig_offset);
                    }
                }
                puts("]\n },");
            }
        }

        // Print configuration data to become a colorbox pop-up

        puts(flot[2]);

        ccparsPrintReport(stdout);

        puts(flot[3]);
    }
}
/*---------------------------------------------------------------------------------------------------------*\
  End of file: ccsigs.c
\*---------------------------------------------------------------------------------------------------------*/


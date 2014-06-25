/*---------------------------------------------------------------------------------------------------------*\
  File:     ccSigs.c                                                                    Copyright CERN 2014

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
#include <math.h>

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRun.h"
#include "ccSigs.h"
#include "flot.h"

#define DIG_STEP        0.5      // Digital signal step size

static unsigned flot_index;      // Index into flot buffers
static float    dig_offset;      // Offset to stack digital signals for FGCSPY and LVDV output formats

/*---------------------------------------------------------------------------------------------------------*/
static void ccSigsEnableSignal(enum ccsig_idx idx)
/*---------------------------------------------------------------------------------------------------------*\
  This function will enable a signal and define its type to be ANALOG, DIGITAL or CURSOR. If the
  CSV output format is FGCSPY or LVDV then for each new digital signal the offset is moved down by -1.0
  so that they do not overlap on the graphing tool when looking at the results.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Enable signal

    signals[idx].control = CC_ENABLED;

    // Set offset for digital signals for FGCSPY and LVDV output formats

    if(signals[idx].type == DIGITAL &&
       (ccpars_global.csv_format == CC_FGCSPY ||
        ccpars_global.csv_format == CC_LVDV))
    {
        dig_offset -= 1.0;

        signals[idx].dig_offset = dig_offset;
    }

    // If FLOT output enabled then allocate buffer memory for non-Cursor signals

    if(ccpars_global.flot_output == CC_ENABLED && signals[idx].type != CURSOR && signals[idx].buf == NULL)
    {
        // Allocate space for overflow point since flot_index will stop at MAX_FLOT_POINTS

        signals[idx].buf = (float *)calloc(MAX_FLOT_POINTS+1, sizeof(double));  // On Mac, floats are doubles
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccSigsStoreAnalog(enum ccsig_idx idx, float ana_value)
/*---------------------------------------------------------------------------------------------------------*\
  This function will store an analog signal value that was previously enabled by ccSigsEnableSignal().
\*---------------------------------------------------------------------------------------------------------*/
{
    if(signals[idx].type != ANALOG)
    {
        fprintf(stderr,"Fatal: Attempt to store an analog value in signal %s which is not enabled as ANALOG\n",
                        signals[idx].name);
        exit(EXIT_FAILURE);
    }

    // Store analogue value if signal is enabled

    if(signals[idx].control == CC_ENABLED)
    {
        // If value is bad then store zero and increment bad value counter

        if(!isfinite(ana_value) || fabs(ana_value) > 1.0E6)
        {
            ana_value = 0.0;
            signals[idx].num_bad_values++;
        }

        signals[idx].value = ana_value;

        // If FLOT output enabled then also save value in the FLOT buffer

        if(ccpars_global.flot_output == CC_ENABLED)
        {
            signals[idx].buf[flot_index] = ana_value;
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccSigsStoreDigital(enum ccsig_idx idx, uint32_t dig_value)
/*---------------------------------------------------------------------------------------------------------*\
  This function will store a digital signal value that was previously enabled by ccSigsEnableSignal().
\*---------------------------------------------------------------------------------------------------------*/
{
    if(signals[idx].type != DIGITAL)
    {
        fprintf(stderr,"Fatal: Attempt to store a digital value in signal %s which is not enabled as DIGITAL\n",
                        signals[idx].name);
        exit(EXIT_FAILURE);
    }

    // Store digital level as a float using the digital offset, if signal is enabled

    if(signals[idx].control == CC_ENABLED)
    {
        signals[idx].value = signals[idx].dig_offset;       // Initialise value to digital zero

        if(dig_value != 0)
        {
            // For digital one, the step depends on the output format

            if(ccpars_global.csv_format == CC_STANDARD)
            {
                signals[idx].value += 1.0;
            }
            else
            {
                signals[idx].value += DIG_STEP;
            }
        }

        // If FLOT output enabled then also save value in the FLOT buffer

        if(ccpars_global.flot_output == CC_ENABLED)
        {
            signals[idx].buf[flot_index] = signals[idx].value;
        }
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccSigsStoreCursor(enum ccsig_idx idx, char *cursor_label)
/*---------------------------------------------------------------------------------------------------------*\
  This function will store a cursor signal value that was previously enabled by ccSigsEnableSignal().
\*---------------------------------------------------------------------------------------------------------*/
{
    if(signals[idx].type != CURSOR)
    {
        fprintf(stderr,"Fatal: Attempt to store a cursor value in signal %s which is not enabled as CURSOR\n",
                        signals[idx].name);
        exit(EXIT_FAILURE);
    }

    // Store the label

    signals[idx].cursor_label = cursor_label;
}
/*---------------------------------------------------------------------------------------------------------*/
void ccSigsInit(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will enable the signals that need to be stored according to the mode(s) of the run.
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    idx;

    // Reset FLOT buffer index

    flot_index = 0;

    // Start with all signals disabled and reset bad values counter

    for(idx = 0 ; idx < NUM_SIGNALS ; idx++)
    {
        signals[idx].control        = CC_DISABLED;
        signals[idx].num_bad_values = 0;
        signals[idx].time_offset    = 0.0;
    }

    // Voltage reference is always enabled

    ccSigsEnableSignal(ANA_V_REF);

    // Enable additional signals with simulating load

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        // Set time offset for circuit simulation signals - if the voltage source is under-sampled then
        // include the steady ramp delay, otherwise the dynamic response is actually simulated

        signals[ANA_B_MAGNET ].time_offset =
        signals[ANA_I_MAGNET ].time_offset =
        signals[ANA_I_CIRCUIT].time_offset =
        signals[ANA_V_CIRCUIT].time_offset = ccpars_global.iter_period * (ccpars_vs.v_ref_delay_iters +
                                            (reg.sim_load_pars.vs_undersampled_flag == 0 ? 0.0 : reg.sim_vs_pars.vs_delay_iters));

        // Enable cursor signals only if CSV output is for the Labview Dataviewer (LVDV)

        if(ccpars_global.csv_format == CC_LVDV)
        {
            ccSigsEnableSignal(CSR_FUNC);
        }

        // Voltage source simulation signals

        ccSigsEnableSignal(ANA_V_REF_LIMITED);
        ccSigsEnableSignal(ANA_V_CIRCUIT);
        ccSigsEnableSignal(ANA_V_MEAS);
        ccSigsEnableSignal(ANA_V_ERR);
        ccSigsEnableSignal(ANA_MAX_ABS_V_ERR);

        ccSigsEnableSignal(DIG_V_REF_CLIP);
        ccSigsEnableSignal(DIG_V_REF_RATE_CLIP);

        if(ccpars_limits.v_err_warning > 0.0)
        {
            ccSigsEnableSignal(DIG_V_REG_ERR_WARN);
        }

        if(ccpars_limits.v_err_fault > 0.0)
        {
            ccSigsEnableSignal(DIG_V_REG_ERR_FLT);
        }

        // Field regulation signals

        if(ccrun.breg_flag == 1)
        {
            ccSigsEnableSignal(ANA_REG_MEAS);
            ccSigsEnableSignal(ANA_TRACK_DLY);
            ccSigsEnableSignal(ANA_B_REF);
            ccSigsEnableSignal(ANA_B_REF_LIMITED);
            ccSigsEnableSignal(ANA_B_REF_RST);
            ccSigsEnableSignal(ANA_B_MAGNET);
            ccSigsEnableSignal(ANA_B_MEAS);
            ccSigsEnableSignal(ANA_B_MEAS_FLTR);
            ccSigsEnableSignal(ANA_B_MEAS_EXTR);
            ccSigsEnableSignal(ANA_B_ERR);
            ccSigsEnableSignal(ANA_MAX_ABS_B_ERR);
            ccSigsEnableSignal(DIG_B_MEAS_TRIP);
            ccSigsEnableSignal(DIG_B_MEAS_LOW);
            ccSigsEnableSignal(DIG_B_MEAS_ZERO);
            ccSigsEnableSignal(DIG_B_REF_CLIP);
            ccSigsEnableSignal(DIG_B_REF_RATE_CLIP);

            if(ccpars_limits.b_err_warning > 0.0)
            {
                ccSigsEnableSignal(DIG_B_REG_ERR_WARN);
            }

            if(ccpars_limits.b_err_fault > 0.0)
            {
                ccSigsEnableSignal(DIG_B_REG_ERR_FLT);
            }
        }

        // Current regulation signals

        if(ccrun.ireg_flag == 1)
        {
            ccSigsEnableSignal(ANA_REG_MEAS);
            ccSigsEnableSignal(ANA_TRACK_DLY);
            ccSigsEnableSignal(ANA_I_REF);
            ccSigsEnableSignal(ANA_I_REF_LIMITED);
            ccSigsEnableSignal(ANA_I_REF_RST);
            ccSigsEnableSignal(ANA_I_ERR);
            ccSigsEnableSignal(ANA_MAX_ABS_I_ERR);
            ccSigsEnableSignal(ANA_V_REF_SAT);
            ccSigsEnableSignal(DIG_I_REF_CLIP);
            ccSigsEnableSignal(DIG_I_REF_RATE_CLIP);

            if(ccpars_limits.i_err_warning > 0.0)
            {
                ccSigsEnableSignal(DIG_I_REG_ERR_WARN);
            }

            if(ccpars_limits.i_err_fault > 0.0)
            {
                ccSigsEnableSignal(DIG_I_REG_ERR_FLT);
            }
        }

        // Current simulation signals

        if(reg.sim_load_pars.load_undersampled_flag == 0)
        {
            ccSigsEnableSignal(ANA_I_MAGNET);
        }
        ccSigsEnableSignal(ANA_I_CIRCUIT);
        ccSigsEnableSignal(ANA_I_MEAS);
        ccSigsEnableSignal(ANA_I_MEAS_FLTR);
        ccSigsEnableSignal(ANA_I_MEAS_EXTR);
        ccSigsEnableSignal(DIG_I_MEAS_TRIP);
        ccSigsEnableSignal(DIG_I_MEAS_LOW);
        ccSigsEnableSignal(DIG_I_MEAS_ZERO);

        // RMS current signals

        if(ccpars_limits.i_rms_tc > 0.0)
        {
            ccSigsEnableSignal(ANA_I_RMS);

            if(ccpars_limits.i_rms_warning > 0.0)
            {
                ccSigsEnableSignal(DIG_I_RMS_WARN);
            }

            if(ccpars_limits.i_rms_fault > 0.0)
            {
                ccSigsEnableSignal(DIG_I_RMS_FLT);
            }
        }
    }

    // If CSV output is enabled, write header to CSV file

    if(ccpars_global.csv_format != CC_NONE)
    {
        // First row: print enabled signal headers
        // Add _D suffix for digital signals if output is for FGCSPY

        fputs("TIME",cctest.csv_file);

        for(idx = 0 ; idx < NUM_SIGNALS ; idx++)
        {
            if(signals[idx].control == CC_ENABLED)
            {
                fprintf(cctest.csv_file,",%s%s", signals[idx].name,
                        ccpars_global.csv_format == CC_FGCSPY &&
                        signals[idx].meta_data[0] == 'T' ? "_D" : "");
            }
        }

        // Second row: if CSV output is for the Labview Dataviewer (LVDV) then add meta data line

        if(ccpars_global.csv_format == CC_LVDV)
        {
            fputs("\nMETA",cctest.csv_file);

            for(idx = 0 ; idx < NUM_SIGNALS ; idx++)
            {
                if(signals[idx].control == CC_ENABLED)
                {
                    fprintf(cctest.csv_file,",%s",signals[idx].meta_data);
                }
            }
        }

        fputc('\n',cctest.csv_file);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccSigsStore(double time)
/*---------------------------------------------------------------------------------------------------------*\
  This function will store all the signals for the current iteration.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Voltage reference is always stored

    ccSigsStoreAnalog(ANA_V_REF, reg.v.ref);

    // Store other signals when simulating load - if they are not enabled then they are ignored

    if(ccpars_global.sim_load == CC_ENABLED)
    {
        switch(reg.mode)
        {
        case REG_FIELD:

            ccSigsStoreAnalog (ANA_B_REF,          reg.ref);
            ccSigsStoreAnalog (ANA_B_REF_LIMITED,  reg.ref_limited);
            ccSigsStoreAnalog (ANA_B_REF_RST,      reg.ref_rst);
            ccSigsStoreAnalog (ANA_I_REF,          0.0);
            ccSigsStoreAnalog (ANA_I_REF_LIMITED,  0.0);
            ccSigsStoreAnalog (ANA_I_REF_RST,      0.0);
            break;

        case REG_CURRENT:


            ccSigsStoreAnalog (ANA_B_REF,          0.0);
            ccSigsStoreAnalog (ANA_B_REF_LIMITED,  0.0);
            ccSigsStoreAnalog (ANA_B_REF_RST,      0.0);
            ccSigsStoreAnalog (ANA_I_REF,          reg.ref);
            ccSigsStoreAnalog (ANA_I_REF_LIMITED,  reg.ref_limited);
            ccSigsStoreAnalog (ANA_I_REF_RST,      reg.ref_rst);
            break;

        default:

            ccSigsStoreAnalog (ANA_B_REF,          0.0);
            ccSigsStoreAnalog (ANA_B_REF_LIMITED,  0.0);
            ccSigsStoreAnalog (ANA_B_REF_RST,      0.0);
            ccSigsStoreAnalog (ANA_I_REF,          0.0);
            ccSigsStoreAnalog (ANA_I_REF_LIMITED,  0.0);
            ccSigsStoreAnalog (ANA_I_REF_RST,      0.0);
            break;
        }

        ccSigsStoreAnalog( ANA_B_MAGNET,       reg.sim_load_vars.magnet_field);
        ccSigsStoreAnalog( ANA_B_MEAS,         reg.b.meas.signal[REG_MEAS_UNFILTERED]);
        ccSigsStoreAnalog( ANA_B_MEAS_FLTR,    reg.b.meas.signal[REG_MEAS_FILTERED]);
        ccSigsStoreAnalog( ANA_B_MEAS_EXTR,    reg.b.meas.signal[REG_MEAS_EXTRAPOLATED]);

        ccSigsStoreAnalog (ANA_I_MAGNET,       reg.sim_load_vars.magnet_current);
        ccSigsStoreAnalog (ANA_I_CIRCUIT,      reg.sim_load_vars.circuit_current);
        ccSigsStoreAnalog (ANA_I_RMS,          sqrtf(reg.i.lim_meas.meas2_filter));
        ccSigsStoreAnalog (ANA_I_MEAS,         reg.i.meas.signal[REG_MEAS_UNFILTERED]);
        ccSigsStoreAnalog (ANA_I_MEAS_FLTR,    reg.i.meas.signal[REG_MEAS_FILTERED]);
        ccSigsStoreAnalog (ANA_I_MEAS_EXTR,    reg.i.meas.signal[REG_MEAS_EXTRAPOLATED]);

        ccSigsStoreAnalog( ANA_REG_MEAS,       reg.meas);

        ccSigsStoreAnalog (ANA_V_REF_SAT,      reg.v.ref_sat);
        ccSigsStoreAnalog (ANA_V_REF_LIMITED,  reg.v.ref_limited);
        ccSigsStoreAnalog (ANA_V_CIRCUIT,      reg.sim_load_vars.circuit_voltage);
//        ccSigsStoreAnalog (ANA_V_MEAS,         reg.v.meas);
        ccSigsStoreAnalog (ANA_V_MEAS,         ccpars_ramp.ramp_pars.time_shift);

        ccSigsStoreAnalog( ANA_TRACK_DLY,      reg.rst_vars.meas_track_delay_periods);

        ccSigsStoreAnalog (ANA_B_ERR,          reg.b.err.err);
        ccSigsStoreAnalog (ANA_I_ERR,          reg.i.err.err);
//        ccSigsStoreAnalog (ANA_V_ERR,          reg.v.err.err);
        ccSigsStoreAnalog (ANA_V_ERR,          ccpars_ramp.ramp_pars.linear_rate_limit);

        ccSigsStoreAnalog (ANA_MAX_ABS_B_ERR,  reg.b.err.max_abs_err);
        ccSigsStoreAnalog (ANA_MAX_ABS_I_ERR,  reg.i.err.max_abs_err);
//        ccSigsStoreAnalog (ANA_MAX_ABS_V_ERR,  reg.v.err.max_abs_err);
        ccSigsStoreAnalog (ANA_MAX_ABS_V_ERR,  ccpars_ramp.ramp_pars.prev_ramp_ref);

        ccSigsStoreDigital(DIG_B_MEAS_TRIP,    reg.b.lim_meas.flags.trip);
        ccSigsStoreDigital(DIG_B_MEAS_LOW,     reg.b.lim_meas.flags.low);
        ccSigsStoreDigital(DIG_B_MEAS_ZERO,    reg.b.lim_meas.flags.zero);

        ccSigsStoreDigital(DIG_B_REF_CLIP,     reg.b.lim_ref.flags.clip);
        ccSigsStoreDigital(DIG_B_REF_RATE_CLIP,reg.b.lim_ref.flags.rate);
        ccSigsStoreDigital(DIG_B_REG_ERR_WARN, reg.b.err.warning.flag);
        ccSigsStoreDigital(DIG_B_REG_ERR_FLT,  reg.b.err.fault.flag);

        ccSigsStoreDigital(DIG_I_MEAS_TRIP, reg.i.lim_meas.flags.trip);
        ccSigsStoreDigital(DIG_I_MEAS_LOW,  reg.i.lim_meas.flags.low);
        ccSigsStoreDigital(DIG_I_MEAS_ZERO, reg.i.lim_meas.flags.zero);

        ccSigsStoreDigital(DIG_I_RMS_WARN,     reg.i.lim_meas.flags.rms_warning);
        ccSigsStoreDigital(DIG_I_RMS_FLT,      reg.i.lim_meas.flags.rms_trip);

        ccSigsStoreDigital(DIG_I_REF_CLIP,     reg.i.lim_ref.flags.clip);
        ccSigsStoreDigital(DIG_I_REF_RATE_CLIP,reg.i.lim_ref.flags.rate);
        ccSigsStoreDigital(DIG_I_REG_ERR_WARN, reg.i.err.warning.flag);
        ccSigsStoreDigital(DIG_I_REG_ERR_FLT,  reg.i.err.fault.flag);

        ccSigsStoreDigital(DIG_V_REG_ERR_FLT,  reg.v.err.fault.flag);
        ccSigsStoreDigital(DIG_V_REG_ERR_WARN, reg.v.err.warning.flag);
        ccSigsStoreDigital(DIG_V_REF_CLIP,     reg.v.lim_ref.flags.clip);
        ccSigsStoreDigital(DIG_V_REF_RATE_CLIP,reg.v.lim_ref.flags.rate);
    }

    // Increment FLOT data index, but clip to max number of FLOT points

    if(flot_index < MAX_FLOT_POINTS)
    {
        flot_index++;
    }

    // If CSV output is enabled, write data to CSV file

    if(ccpars_global.csv_format != CC_NONE)
    {
        uint32_t idx;

        // Print the timestamp first with microsecond resolution

        fprintf(cctest.csv_file,"%.6f",time);

        // Print enabled signal values

        for(idx = 0 ; idx < NUM_SIGNALS ; idx++)
        {
            if(signals[idx].control == CC_ENABLED)
            {
                fputc(',',cctest.csv_file);

                switch(signals[idx].type)
                {
                case ANALOG:

                    fprintf(cctest.csv_file,"%.7E",signals[idx].value);
                    break;

                case DIGITAL:

                    fprintf(cctest.csv_file,"%.1f",signals[idx].value);
                    break;

                case CURSOR:        // Cursor values - clear cursor label after printing

                    if(signals[idx].cursor_label != NULL)
                    {
                        fputs(signals[idx].cursor_label, cctest.csv_file);
                        signals[idx].cursor_label = NULL;
                    }
                    break;
                }
            }
        }

        fputc('\n',cctest.csv_file);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccSigsFlotInvalidSignal(FILE *f, enum ccsig_idx sig_idx, uint32_t *n_points, char label)
{
    double         time;
    uint32_t       iteration_idx;

    if(signals[sig_idx].control == CC_ENABLED)
    {
        fprintf(f,"\"INVALID_%c\": { lines: { show:false }, points: { show:true },\ndata:[", label);

        for(iteration_idx = 0; iteration_idx < flot_index; iteration_idx++)
        {
            // Only print changed values when meta_data is TRAIL_STEP

            if((iteration_idx % ccpars_meas.invalid_meas_period_iters) < ccpars_meas.invalid_meas_repeat_iters)
            {
                time = reg.iter_period * iteration_idx;

                fprintf(f,"[%.6f,%.7E],", time, signals[sig_idx].buf[iteration_idx]);
                (*n_points)++;
            }
        }
        fputs("]\n },\n",f);
    }
}
/*---------------------------------------------------------------------------------------------------------*/
void ccSigsFlot(FILE *f)
/*---------------------------------------------------------------------------------------------------------*\
  This function will print the flot data and footer
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t       func_idx;
    uint32_t       iteration_idx;
    uint32_t       sig_idx;
    uint32_t       n_points = 0;
    double         time;
    double         start_func_time;
    float          time_offset;
    struct cccmds *cmd;

    // Warn user if FLOT data was truncated

    if(flot_index >= MAX_FLOT_POINTS)
    {
        printf("Warning - FLOT data truncated to %u points\n",MAX_FLOT_POINTS);
    }

    // Print start of FLOT html page including flot path to all the javascript libraries

    fprintf(f,flot[0],FLOT_PATH,FLOT_PATH,FLOT_PATH,FLOT_PATH,FLOT_PATH,FLOT_PATH,FLOT_PATH);

    // Time of end of FLOT data

    start_func_time = 0.0;

    // For each function

    for(func_idx = 0 ; func_idx < ccrun.num_functions ; func_idx++)
    {
        // Print table, pppl or plep data if selected and use points instead of lines

        if(ccpars_global.function[func_idx] == FG_TABLE ||
           ccpars_global.function[func_idx] == FG_PPPL ||
           ccpars_global.function[func_idx] == FG_PLEP)
        {
            fprintf(f,"\"%u.%s\": { lines: { show:false }, points: { show:true },\ndata:[",
                      func_idx+1, ccParsEnumString(function_type, ccpars_global.function[func_idx]));

            switch(ccpars_global.function[func_idx])
            {
            default: break;     // Suppress gcc warning

            case FG_TABLE:

                for(iteration_idx = 0 ; iteration_idx < table_pars[0].num_elements ; iteration_idx++)
                {
                    time = start_func_time + ccpars_global.run_delay + ccpars_table.time[iteration_idx];

                    fprintf(f,"[%.6f,%.7E],", time, ccpars_table.ref[iteration_idx]);
                    n_points++;
                }
                break;

            case FG_PPPL:

                time = start_func_time + ccpars_global.run_delay;

                fprintf(f,"[%.6f,%.7E],", time, ccpars_pppl.initial_ref);
                n_points++;

                for(iteration_idx = 0 ; iteration_idx < ccpars_pppl.pppl_pars.num_segs ; iteration_idx++)
                {
                    time = start_func_time + ccpars_pppl.pppl_pars.time[iteration_idx];

                    fprintf(f,"[%.6f,%.7E],", time, ccpars_pppl.pppl_pars.a0[iteration_idx]);
                    n_points++;
                }
                break;

            case FG_PLEP:

                time = start_func_time + ccpars_global.run_delay;

                fprintf(f,"[%.6f,%.7E],", time, ccpars_plep.initial_ref);
                n_points++;

                for(iteration_idx = 0 ; iteration_idx <= FG_PLEP_N_SEGS ; iteration_idx++)
                {
                    time = start_func_time + ccpars_plep.plep_pars.time[iteration_idx];

                    fprintf(f,"[%.6f,%.7E],", time, ccpars_plep.plep_pars.normalisation * ccpars_plep.plep_pars.ref[iteration_idx]);
                    n_points++;
                }
                break;
            }

            fputs("]\n },\n",f);
        }

        start_func_time += ccrun.fg_meta[func_idx].duration + ccpars_global.pre_func_delay;
    }

    // Highlight invalid points if enabled

    if(ccpars_meas.invalid_meas_period_iters > 0 && ccpars_meas.invalid_meas_repeat_iters > 0)
    {
        ccSigsFlotInvalidSignal(f, ANA_B_MEAS, &n_points, 'B');
        ccSigsFlotInvalidSignal(f, ANA_I_MEAS, &n_points, 'I');
        ccSigsFlotInvalidSignal(f, ANA_V_MEAS, &n_points, 'V');
    }

    // Print enabled analog signal values

    for(sig_idx = 0 ; sig_idx < NUM_SIGNALS ; sig_idx++)
    {
        if(signals[sig_idx].control == CC_ENABLED && signals[sig_idx].type == ANALOG)
        {
            time_offset = signals[sig_idx].time_offset;

            fprintf(f,"\"%s\": { lines: { steps:%s }, points: { show:false },\ndata:[",
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
                        time = reg.iter_period * iteration_idx + time_offset;
                    }
                    else
                    {
                        time = reg.iter_period * (ccrun.num_iterations - iteration_idx - 1);
                    }

                    fprintf(f,"[%.6f,%.7E],", time, signals[sig_idx].buf[iteration_idx]);
                    n_points++;
                }
            }
            fputs("]\n },\n",f);
        }
    }

    // Print start of digital signals

    fputs(flot[1],f);

    // Print enabled digital signal values

    for(sig_idx = 0, dig_offset = -DIG_STEP/2.0 ; sig_idx < NUM_SIGNALS ; sig_idx++)
    {
        if(signals[sig_idx].control == CC_ENABLED && signals[sig_idx].type == DIGITAL)
        {
            dig_offset -= 1.0;

            fprintf(f,"\"%s\": {\n lines: { steps:%s },\n data:[",
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
                        time = reg.iter_period * (ccrun.num_iterations - iteration_idx - 1);
                    }

                    fprintf(f,"[%.6f,%.2f],", time, signals[sig_idx].buf[iteration_idx] + dig_offset);
                    n_points++;
                }
            }
            fputs("]\n },\n",f);
        }
    }

    // Print command parameter values to become a colorbox pop-up

    fprintf(f, flot[2], CC_VERSION);    // Version is embedded in the About pop-up title: "About cctest vx.xx"

    for(cmd = cmds ; cmd->name != NULL ; cmd++)
    {
        if(cmd->enabled == 1)
        {
            fputc('\n',f);
            ccParsPrintAll(f, cmd->name, cmd->pars);
        }
    }

    // Print debug variable to become a colorbox pop-up

    fputs(flot[3],f);

    fprintf(f,"%-*s %u\n\n", PARS_INDENT, "FLOT:n_points", n_points);

    ccParsPrintDebug(f);

    // Write HTML file footer

    fputs(flot[4],f);
}
/*---------------------------------------------------------------------------------------------------------*/
uint32_t ccSigsReportBadValues(void)
/*---------------------------------------------------------------------------------------------------------*\
  This function will report the number of bad values submitted for all signals
\*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t    idx;
    uint32_t    exit_status = EXIT_SUCCESS;

    // Start with all signals disabled and reset bad values counter

    for(idx = 0 ; idx < NUM_SIGNALS ; idx++)
    {
        if(signals[idx].control == CC_ENABLED && signals[idx].num_bad_values > 0)
        {
            printf("Bad values for %-20s : %6u\n",signals[idx].name, signals[idx].num_bad_values);
            exit_status = EXIT_FAILURE;
        }
    }

    return(exit_status);
}

// EOF

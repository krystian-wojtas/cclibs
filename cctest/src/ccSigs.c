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

#include "ccCmds.h"
#include "ccTest.h"
#include "ccRun.h"
#include "ccSigs.h"
#include "ccFlot.h"

static float    dig_offset;      // Offset to stack digital signals for FGCSPY and LVDV output formats

/*---------------------------------------------------------------------------------------------------------*/
void ccSigsEnableSignal(enum ccsig_idx idx)
/*---------------------------------------------------------------------------------------------------------*\
  This function will enable a signal and define its type to be ANALOG, DIGITAL or CURSOR. If the
  CSV output format is FGCSPY or LVDV then for each new digital signal the offset is moved down by -1.0
  so that they do not overlap on the graphing tool when looking at the results.
\*---------------------------------------------------------------------------------------------------------*/
{
    // Enable signal

    signals[idx].control = REG_ENABLED;

    // Set offset for digital signals for FGCSPY and LVDV output formats

    if(signals[idx].type == DIGITAL &&
       (ccpars_global.csv_format == CC_FGCSPY ||
        ccpars_global.csv_format == CC_LVDV))
    {
        dig_offset -= 1.0;

        signals[idx].dig_offset = dig_offset;
    }

    // If FLOT output enabled then allocate buffer memory for non-Cursor signals

    if(ccpars_global.flot_output == REG_ENABLED && signals[idx].type != CURSOR && signals[idx].buf == NULL)
    {
        // Allocate space for overflow point since flot_index will stop at FLOT_POINTS_MAX

        signals[idx].buf = (float *)calloc(ccpars_global.flot_points_max+1, sizeof(double));  // On Mac, floats are doubles
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

    if(signals[idx].control == REG_ENABLED)
    {
        // If value is bad then store zero and increment bad value counter

        if(!isfinite(ana_value) || fabs(ana_value) > 1.0E6)
        {
            ana_value = 0.0;
            signals[idx].num_bad_values++;
        }

        signals[idx].value = ana_value;

        // If FLOT output enabled then also save value in the FLOT buffer

        if(ccpars_global.flot_output == REG_ENABLED)
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

    if(signals[idx].control == REG_ENABLED)
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

        if(ccpars_global.flot_output == REG_ENABLED)
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

    flot_index = 0;
    dig_offset = 0.0;

    // Start with all signals disabled and reset bad values counter

    for(idx = 0 ; idx < NUM_SIGNALS ; idx++)
    {
        signals[idx].control        = REG_DISABLED;
        signals[idx].num_bad_values = 0;
        signals[idx].time_offset    = 0.0;
    }

    // Voltage reference is always enabled when converter actuation is voltage ref

    if(ccpars_pc.actuation == REG_VOLTAGE_REF)
    {
        ccSigsEnableSignal(ANA_V_REF);
    }

    // Enable additional signals with simulating load

    if(ccpars_global.sim_load == REG_ENABLED)
    {
        // Set time offset for circuit simulation signals - if the voltage source is under-sampled then
        // include the steady ramp delay, otherwise the dynamic response is actually simulated

        signals[ANA_B_MAGNET ].time_offset =
        signals[ANA_I_MAGNET ].time_offset =
        signals[ANA_I_CIRCUIT].time_offset =
        signals[ANA_V_CIRCUIT].time_offset = conv.iter_period * (ccpars_pc.act_delay_iters +
                                            (conv.sim_pc_pars.is_pc_undersampled == 0 ? 0.0 : conv.sim_pc_pars.rsp_delay_iters));

        // Enable cursor signals only if CSV output is for the Labview Dataviewer (LVDV)

        if(ccpars_global.csv_format == CC_LVDV)
        {
            ccSigsEnableSignal(CSR_FUNC);
        }

        if(ccpars_pc.actuation == REG_VOLTAGE_REF)
        {
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

            if(ccrun.is_breg_enabled == true)
            {
                ccSigsEnableSignal(ANA_REG_MEAS);
                ccSigsEnableSignal(ANA_TRACK_DLY);
                ccSigsEnableSignal(ANA_B_REF);
                ccSigsEnableSignal(ANA_B_REF_LIMITED);
                ccSigsEnableSignal(ANA_B_REF_RST);
                ccSigsEnableSignal(ANA_B_REF_DELAYED);
                ccSigsEnableSignal(ANA_B_MAGNET);
                ccSigsEnableSignal(ANA_B_MEAS);
                ccSigsEnableSignal(ANA_B_MEAS_FLTR);
                ccSigsEnableSignal(ANA_B_MEAS_EXTR);
                ccSigsEnableSignal(ANA_B_ERR);
                ccSigsEnableSignal(ANA_MAX_ABS_B_ERR);
                ccSigsEnableSignal(DIG_B_MEAS_TRIP);
                ccSigsEnableSignal(DIG_B_REF_CLIP);
                ccSigsEnableSignal(DIG_B_REF_RATE_CLIP);

                if(ccpars_limits.b_neg[ccpars_load.select] >= 0.0)
                {
                    ccSigsEnableSignal(ANA_B_REF_OPENLOOP);
                }

                if(ccpars_limits.b_low[ccpars_load.select] > 0.0)
                {
                    ccSigsEnableSignal(DIG_B_MEAS_LOW);
                }

                if(ccpars_limits.b_zero[ccpars_load.select] > 0.0)
                {
                    ccSigsEnableSignal(DIG_B_MEAS_ZERO);
                }

                if(ccpars_limits.b_err_warning[ccpars_load.select] > 0.0)
                {
                    ccSigsEnableSignal(DIG_B_REG_ERR_WARN);
                }

                if(ccpars_limits.b_err_fault[ccpars_load.select] > 0.0)
                {
                    ccSigsEnableSignal(DIG_B_REG_ERR_FLT);
                }
            }

            // Current regulation signals

            if(ccrun.is_ireg_enabled == true)
            {
                ccSigsEnableSignal(ANA_REG_MEAS);
                ccSigsEnableSignal(ANA_TRACK_DLY);
                ccSigsEnableSignal(ANA_I_REF);
                ccSigsEnableSignal(ANA_I_REF_LIMITED);
                ccSigsEnableSignal(ANA_I_REF_RST);
                ccSigsEnableSignal(ANA_I_REF_DELAYED);
                ccSigsEnableSignal(ANA_I_ERR);
                ccSigsEnableSignal(ANA_MAX_ABS_I_ERR);
                ccSigsEnableSignal(ANA_V_REF_SAT);
                ccSigsEnableSignal(DIG_I_REF_CLIP);
                ccSigsEnableSignal(DIG_I_REF_RATE_CLIP);

                if(ccpars_limits.i_neg[ccpars_load.select] >= 0.0)
                {
                    ccSigsEnableSignal(ANA_I_REF_OPENLOOP);
                }

                if(ccpars_limits.i_err_warning[ccpars_load.select] > 0.0)
                {
                    ccSigsEnableSignal(DIG_I_REG_ERR_WARN);
                }

                if(ccpars_limits.i_err_fault[ccpars_load.select] > 0.0)
                {
                    ccSigsEnableSignal(DIG_I_REG_ERR_FLT);
                }
            }

            // Current simulation signals

            if(conv.sim_load_pars.is_load_undersampled == false)
            {
                ccSigsEnableSignal(ANA_I_MAGNET);
            }
        }
        else // Converter actuation is CURRENT reference
        {
            ccSigsEnableSignal(ANA_REG_MEAS);
            ccSigsEnableSignal(ANA_V_CIRCUIT);
            ccSigsEnableSignal(ANA_V_MEAS);
            ccSigsEnableSignal(ANA_I_REF);
            ccSigsEnableSignal(ANA_I_REF_LIMITED);
            ccSigsEnableSignal(ANA_I_REF_DELAYED);
            ccSigsEnableSignal(DIG_I_REF_CLIP);

            if(ccpars_limits.i_rate[ccpars_load.select] > 0.0)
            {
                ccSigsEnableSignal(DIG_I_REF_RATE_CLIP);
            }

            signals[ANA_REG_MEAS].time_offset = -conv.iter_period * (uint32_t)(conv.i.meas.delay_iters[ccpars_meas.i_reg_select] + 0.499);
        }

        // Current simulation signals

        ccSigsEnableSignal(ANA_I_CIRCUIT);
        ccSigsEnableSignal(ANA_I_MEAS);
        ccSigsEnableSignal(ANA_I_MEAS_FLTR);
        ccSigsEnableSignal(ANA_I_MEAS_EXTR);
        ccSigsEnableSignal(DIG_I_MEAS_TRIP);

        if(ccpars_limits.i_low[ccpars_load.select] > 0.0)
        {
            ccSigsEnableSignal(DIG_I_MEAS_LOW);
        }

        if(ccpars_limits.i_zero[ccpars_load.select] > 0.0)
        {
            ccSigsEnableSignal(DIG_I_MEAS_ZERO);
        }

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

        // RMS_LOAD current signals

        if(ccpars_limits.i_rms_load_tc[ccpars_load.select] > 0.0)
        {
            ccSigsEnableSignal(ANA_I_RMS_LOAD);

            if(ccpars_limits.i_rms_load_warning[ccpars_load.select] > 0.0)
            {
                ccSigsEnableSignal(DIG_I_RMS_LOAD_WARN);
            }

            if(ccpars_limits.i_rms_load_fault[ccpars_load.select] > 0.0)
            {
                ccSigsEnableSignal(DIG_I_RMS_LOAD_FLT);
            }
        }

        // INVALID_MEAS signal

        if(ccrun.invalid_meas.random_threshold > 0)
        {
            ccSigsEnableSignal(DIG_INVALID_MEAS);
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
            if(signals[idx].control == REG_ENABLED)
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
                if(signals[idx].control == REG_ENABLED)
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

    ccSigsStoreAnalog(ANA_V_REF, conv.v.ref);

    // Store other signals when simulating load - if they are not enabled then they are ignored

    if(ccpars_global.sim_load == REG_ENABLED)
    {
        if(ccpars_pc.actuation == REG_CURRENT_REF)
        {
            ccSigsStoreAnalog (ANA_I_REF,          conv.ref);
            ccSigsStoreAnalog (ANA_I_REF_LIMITED,  conv.ref_limited);
            ccSigsStoreAnalog (ANA_I_REF_DELAYED,  conv.ref_delayed);
        }
        else // Actuation is VOLTAGE_REF
        {
            switch(conv.reg_mode)
            {
            case REG_FIELD:

                ccSigsStoreAnalog (ANA_B_REF,          conv.ref);
                ccSigsStoreAnalog (ANA_B_REF_LIMITED,  conv.ref_limited);
                ccSigsStoreAnalog (ANA_B_REF_RST,      conv.ref_rst);
                ccSigsStoreAnalog (ANA_B_REF_OPENLOOP, conv.ref_openloop);
                ccSigsStoreAnalog (ANA_B_REF_DELAYED,  conv.ref_delayed);

                ccSigsStoreAnalog (ANA_I_REF,          0.0);
                ccSigsStoreAnalog (ANA_I_REF_LIMITED,  0.0);
                ccSigsStoreAnalog (ANA_I_REF_RST,      0.0);
                ccSigsStoreAnalog (ANA_I_REF_OPENLOOP, 0.0);
                ccSigsStoreAnalog (ANA_I_REF_DELAYED,  0.0);
                break;

            case REG_CURRENT:


                ccSigsStoreAnalog (ANA_B_REF,          0.0);
                ccSigsStoreAnalog (ANA_B_REF_LIMITED,  0.0);
                ccSigsStoreAnalog (ANA_B_REF_RST,      0.0);
                ccSigsStoreAnalog (ANA_B_REF_OPENLOOP, 0.0);
                ccSigsStoreAnalog (ANA_B_REF_DELAYED,  0.0);

                ccSigsStoreAnalog (ANA_I_REF,          conv.ref);
                ccSigsStoreAnalog (ANA_I_REF_LIMITED,  conv.ref_limited);
                ccSigsStoreAnalog (ANA_I_REF_RST,      conv.ref_rst);
                ccSigsStoreAnalog (ANA_I_REF_OPENLOOP, conv.ref_openloop);
                ccSigsStoreAnalog (ANA_I_REF_DELAYED,  conv.ref_delayed);
                break;

            case REG_VOLTAGE:

                ccSigsStoreAnalog (ANA_B_REF,          0.0);
                ccSigsStoreAnalog (ANA_B_REF_LIMITED,  0.0);
                ccSigsStoreAnalog (ANA_B_REF_RST,      0.0);
                ccSigsStoreAnalog (ANA_B_REF_OPENLOOP, 0.0);
                ccSigsStoreAnalog (ANA_B_REF_DELAYED,  0.0);

                ccSigsStoreAnalog (ANA_I_REF,          0.0);
                ccSigsStoreAnalog (ANA_I_REF_LIMITED,  0.0);
                ccSigsStoreAnalog (ANA_I_REF_RST,      0.0);
                ccSigsStoreAnalog (ANA_I_REF_OPENLOOP, 0.0);
                ccSigsStoreAnalog (ANA_I_REF_DELAYED,  0.0);
                break;

            case REG_NONE:

                break;
            }
        }

        ccSigsStoreAnalog( ANA_B_MAGNET,       conv.sim_load_vars.magnet_field);
        ccSigsStoreAnalog( ANA_B_MEAS,         conv.b.meas.signal[REG_MEAS_UNFILTERED]);
        ccSigsStoreAnalog( ANA_B_MEAS_FLTR,    conv.b.meas.signal[REG_MEAS_FILTERED]);
        ccSigsStoreAnalog( ANA_B_MEAS_EXTR,    conv.b.meas.signal[REG_MEAS_EXTRAPOLATED]);

        ccSigsStoreAnalog (ANA_I_MAGNET,       conv.sim_load_vars.magnet_current);
        ccSigsStoreAnalog (ANA_I_CIRCUIT,      conv.sim_load_vars.circuit_current);
        ccSigsStoreAnalog (ANA_I_RMS,          sqrtf(conv.lim_i_rms.meas2_filter));
        ccSigsStoreAnalog (ANA_I_RMS_LOAD,     sqrtf(conv.lim_i_rms_load.meas2_filter));
        ccSigsStoreAnalog (ANA_I_MEAS,         conv.i.meas.signal[REG_MEAS_UNFILTERED]);
        ccSigsStoreAnalog (ANA_I_MEAS_FLTR,    conv.i.meas.signal[REG_MEAS_FILTERED]);
        ccSigsStoreAnalog (ANA_I_MEAS_EXTR,    conv.i.meas.signal[REG_MEAS_EXTRAPOLATED]);

        ccSigsStoreAnalog( ANA_REG_MEAS,       conv.meas);

        ccSigsStoreAnalog (ANA_V_REF_SAT,      conv.v.ref_sat);
        ccSigsStoreAnalog (ANA_V_REF_LIMITED,  conv.v.ref_limited);
        ccSigsStoreAnalog (ANA_V_CIRCUIT,      conv.sim_load_vars.circuit_voltage);
        ccSigsStoreAnalog (ANA_V_MEAS,         conv.v.meas);

        ccSigsStoreAnalog( ANA_TRACK_DLY,      conv.track_delay_periods);

        ccSigsStoreAnalog (ANA_B_ERR,          conv.b.err.err);
        ccSigsStoreAnalog (ANA_I_ERR,          conv.i.err.err);
        ccSigsStoreAnalog (ANA_V_ERR,          conv.v.err.err);

        ccSigsStoreAnalog (ANA_MAX_ABS_B_ERR,  conv.b.err.max_abs_err);
        ccSigsStoreAnalog (ANA_MAX_ABS_I_ERR,  conv.i.err.max_abs_err);
        ccSigsStoreAnalog (ANA_MAX_ABS_V_ERR,  conv.v.err.max_abs_err);

        ccSigsStoreDigital(DIG_B_MEAS_TRIP,    conv.b.lim_meas.flags.trip);
        ccSigsStoreDigital(DIG_B_MEAS_LOW,     conv.b.lim_meas.flags.low);
        ccSigsStoreDigital(DIG_B_MEAS_ZERO,    conv.b.lim_meas.flags.zero);

        ccSigsStoreDigital(DIG_B_REF_CLIP,     conv.b.lim_ref.flags.clip);
        ccSigsStoreDigital(DIG_B_REF_RATE_CLIP,conv.b.lim_ref.flags.rate);
        ccSigsStoreDigital(DIG_B_REG_ERR_WARN, conv.b.err.warning.flag);
        ccSigsStoreDigital(DIG_B_REG_ERR_FLT,  conv.b.err.fault.flag);

        ccSigsStoreDigital(DIG_I_MEAS_TRIP,    conv.i.lim_meas.flags.trip);
        ccSigsStoreDigital(DIG_I_MEAS_LOW,     conv.i.lim_meas.flags.low);
        ccSigsStoreDigital(DIG_I_MEAS_ZERO,    conv.i.lim_meas.flags.zero);

        ccSigsStoreDigital(DIG_I_RMS_WARN,     conv.lim_i_rms.flags.warning);
        ccSigsStoreDigital(DIG_I_RMS_FLT,      conv.lim_i_rms.flags.fault);
        ccSigsStoreDigital(DIG_I_RMS_LOAD_WARN,conv.lim_i_rms_load.flags.warning);
        ccSigsStoreDigital(DIG_I_RMS_LOAD_FLT, conv.lim_i_rms_load.flags.fault);

        ccSigsStoreDigital(DIG_I_REF_CLIP,     conv.i.lim_ref.flags.clip);
        ccSigsStoreDigital(DIG_I_REF_RATE_CLIP,conv.i.lim_ref.flags.rate);
        ccSigsStoreDigital(DIG_I_REG_ERR_WARN, conv.i.err.warning.flag);
        ccSigsStoreDigital(DIG_I_REG_ERR_FLT,  conv.i.err.fault.flag);

        ccSigsStoreDigital(DIG_V_REG_ERR_FLT,  conv.v.err.fault.flag);
        ccSigsStoreDigital(DIG_V_REG_ERR_WARN, conv.v.err.warning.flag);
        ccSigsStoreDigital(DIG_V_REF_CLIP,     conv.v.lim_ref.flags.clip);
        ccSigsStoreDigital(DIG_V_REF_RATE_CLIP,conv.v.lim_ref.flags.rate);

        ccSigsStoreDigital(DIG_INVALID_MEAS,   ccrun.invalid_meas.flag);
    }

    // Increment FLOT data index, but clip to max number of FLOT points

    if(flot_index < ccpars_global.flot_points_max)
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
            if(signals[idx].control == REG_ENABLED)
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



uint32_t ccSigsReportBadValues(void)
{
    uint32_t    idx;
    uint32_t    exit_status = EXIT_SUCCESS;

    // Start with all signals disabled and reset bad values counter

    for(idx = 0 ; idx < NUM_SIGNALS ; idx++)
    {
        if(signals[idx].control == REG_ENABLED && signals[idx].num_bad_values > 0)
        {
            printf("Bad values for %-20s : %6u\n",signals[idx].name, signals[idx].num_bad_values);
            exit_status = EXIT_FAILURE;
        }
    }

    return(exit_status);
}

// EOF

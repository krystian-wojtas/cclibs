/*---------------------------------------------------------------------------------------------------------*\
  File:     ccDebug.c                                                                   Copyright CERN 2014

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

  Purpose:  Converter controls libraries test program debug reporting functions

  Author:   Quentin.King@cern.ch
\*---------------------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdarg.h>

// Include cctest program header files

#include "ccRef.h"
#include "ccRun.h"
#include "ccPars.h"

/*---------------------------------------------------------------------------------------------------------*/
static char * ccDebugLabel(char *format, ...)
/*---------------------------------------------------------------------------------------------------------*/
{
    va_list     argv;
    uint32_t    num_chars;
    static char debug_label[PARS_INDENT+1];

    // Print debug label to static buffer (this function is not re-entrant)

    va_start(argv, format);
    num_chars = vsnprintf(debug_label, PARS_INDENT+1, format, argv);
    va_end(argv);

    // Pad the rest of the debug label with spaces

    while(num_chars < PARS_INDENT)
    {
        debug_label[num_chars++] = ' ';
    }

    debug_label[PARS_INDENT] = '\0';

    return(debug_label);
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccDebugFuncMeta(FILE *f, char *prefix, uint32_t cyc_sel)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t i;

    fprintf(f,"%s  %s\n", ccDebugLabel("%s function(%u)"             , prefix, cyc_sel), ccParsEnumString(enum_function_type,    ccpars_ref.function[cyc_sel][0]));
    fprintf(f,"%s  %s\n", ccDebugLabel("%s reg_mode(%u)"             , prefix, cyc_sel), ccParsEnumString(enum_reg_mode,         ccpars_ref.reg_mode[cyc_sel][0]));
    fprintf(f,"%s  %s\n", ccDebugLabel("%s fg_meta(%u).invert_limits", prefix, cyc_sel), ccParsEnumString(enum_enabled_disabled, ccrun.fg_meta[cyc_sel].invert_limits));

    if(ccrun.fg_meta[cyc_sel].fg_error != FG_OK)
    {
        fprintf(f,"%s  %s\n",                 ccDebugLabel("%s fg_meta(%u).fg_error"   , prefix, cyc_sel), ccParsEnumString(enum_fg_error, ccrun.fg_meta[cyc_sel].fg_error));
        fprintf(f,"%s " PARS_INT_FORMAT "\n", ccDebugLabel("%s fg_meta(%u).error.index", prefix, cyc_sel), ccrun.fg_meta[cyc_sel].error.index);
        fprintf(f,"%s",                       ccDebugLabel("%s fg_meta(%u).error.data ", prefix, cyc_sel));

        for(i = 0 ; i < FG_ERR_DATA_LEN ; i++)
        {
            fprintf(f," " PARS_FLOAT_FORMAT, ccrun.fg_meta[cyc_sel].error.data[i]);
        }

        fputc('\n',f);
    }

    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s fg_meta(%u).duration"   , prefix, cyc_sel), ccrun.fg_meta[cyc_sel].duration   );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s fg_meta(%u).range.start", prefix, cyc_sel), ccrun.fg_meta[cyc_sel].range.start);
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s fg_meta(%u).range.end"  , prefix, cyc_sel), ccrun.fg_meta[cyc_sel].range.end  );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s fg_meta(%u).range.min"  , prefix, cyc_sel), ccrun.fg_meta[cyc_sel].range.min  );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n\n",ccDebugLabel("%s fg_meta(%u).range.max"  , prefix, cyc_sel), ccrun.fg_meta[cyc_sel].range.max  );
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccDebugPrintLoad(FILE *f, char *prefix, struct reg_load_pars *load_pars)
/*---------------------------------------------------------------------------------------------------------*/
{
    // Report internally calculated load parameters

    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s ohms_ser"  , prefix), load_pars->ohms_ser  );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s ohms_par"  , prefix), load_pars->ohms_par  );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s ohms_mag"  , prefix), load_pars->ohms_mag  );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s henrys"    , prefix), load_pars->henrys    );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s inv_henrys", prefix), load_pars->inv_henrys);
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s ohms"      , prefix), load_pars->ohms      );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s ohms1"     , prefix), load_pars->ohms1     );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s ohms2"     , prefix), load_pars->ohms2     );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s tc"        , prefix), load_pars->tc        );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s gain0"     , prefix), load_pars->gain0     );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s gain1"     , prefix), load_pars->gain1     );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s gain2"     , prefix), load_pars->gain2     );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n\n",ccDebugLabel("%s gain3"     , prefix), load_pars->gain3     );

    if(load_pars->sat.i_end > 0.0)
    {
        fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s sat.henrys"  , prefix), load_pars->sat.henrys  );
        fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s sat.i_delta" , prefix), load_pars->sat.i_delta );
        fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s sat.b_end"   , prefix), load_pars->sat.b_end   );
        fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s sat.b_factor", prefix), load_pars->sat.b_factor);
        fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s sat.l_rate"  , prefix), load_pars->sat.l_rate  );
        fprintf(f,"%s " PARS_FLOAT_FORMAT "\n\n",ccDebugLabel("%s sat.l_clip"  , prefix), load_pars->sat.l_clip  );
    }
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccDebugPrintMeas(FILE *f, char *prefix, struct reg_meas_filter *meas_filter)
/*---------------------------------------------------------------------------------------------------------*/
{
    fprintf(f,"%s " PARS_INT_FORMAT   "\n",  ccDebugLabel("%s fir_length[0]"          , prefix), meas_filter->fir_length[0]          );
    fprintf(f,"%s " PARS_INT_FORMAT   "\n",  ccDebugLabel("%s fir_length[1]"          , prefix), meas_filter->fir_length[1]          );
    fprintf(f,"%s " PARS_INT_FORMAT   "\n",  ccDebugLabel("%s fir_length[0]"          , prefix), meas_filter->fir_length[0]          );
    fprintf(f,"%s " PARS_INT_FORMAT   "\n",  ccDebugLabel("%s extrapolation_len_iters", prefix), meas_filter->extrapolation_len_iters);
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s float_to_integer"       , prefix), meas_filter->float_to_integer       );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s integer_to_float"       , prefix), meas_filter->integer_to_float       );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s extrapolation_factor"   , prefix), meas_filter->extrapolation_factor   );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",  ccDebugLabel("%s delay_iters[0]"         , prefix), meas_filter->delay_iters[0]         );
    fprintf(f,"%s " PARS_FLOAT_FORMAT "\n\n",ccDebugLabel("%s delay_iters[1]"         , prefix), meas_filter->delay_iters[1]         );
}
/*---------------------------------------------------------------------------------------------------------*/
static void ccDebugPrintReg(FILE *f, char *prefix, struct reg_rst_pars *rst_pars)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t i;

    fprintf(f,"%s " PARS_INT_FORMAT    "\n", ccDebugLabel("%s status"             , prefix), rst_pars->status                 );
    fprintf(f,"%s " PARS_INT_FORMAT    "\n", ccDebugLabel("%s jurys_result"       , prefix), rst_pars->jurys_result           );
    fprintf(f,"%s " PARS_INT_FORMAT    "\n", ccDebugLabel("%s alg_index"          , prefix), rst_pars->alg_index              );
    fprintf(f,"%s " PARS_INT_FORMAT    "\n", ccDebugLabel("%s dead_beat"          , prefix), rst_pars->dead_beat              );
    fprintf(f,"%s " PARS_FLOAT_FORMAT  "\n", ccDebugLabel("%s modulus_margin"     , prefix), rst_pars->modulus_margin         );
    fprintf(f,"%s " PARS_FLOAT_FORMAT  "\n", ccDebugLabel("%s modulus_margin_freq", prefix), rst_pars->modulus_margin_freq    );

    fprintf(f,"%s " PARS_FLOAT_FORMAT  "\n", ccDebugLabel("%s pure_delay_periods" , prefix), rst_pars->pure_delay_periods     );
    fprintf(f,"%s " PARS_FLOAT_FORMAT  "\n", ccDebugLabel("%s track_delay_periods", prefix), rst_pars->track_delay_periods    );
    fprintf(f,"%s " PARS_FLOAT_FORMAT  "\n", ccDebugLabel("%s ref_delay_periods"  , prefix), rst_pars->ref_delay_periods      );

    fprintf(f,"%s " PARS_DOUBLE_FORMAT "\n", ccDebugLabel("%s openloop_fwd_ref[0]", prefix), rst_pars->openloop_forward.ref[0]);
    fprintf(f,"%s " PARS_DOUBLE_FORMAT "\n", ccDebugLabel("%s openloop_fwd_ref[1]", prefix), rst_pars->openloop_forward.ref[1]);
    fprintf(f,"%s " PARS_DOUBLE_FORMAT "\n", ccDebugLabel("%s openloop_fwd_act[1]", prefix), rst_pars->openloop_forward.act[1]);

    fprintf(f,"%s " PARS_DOUBLE_FORMAT "\n", ccDebugLabel("%s openloop_rev_ref[1]", prefix), rst_pars->openloop_reverse.ref[1]);
    fprintf(f,"%s " PARS_DOUBLE_FORMAT "\n", ccDebugLabel("%s openloop_rev_act[0]", prefix), rst_pars->openloop_reverse.act[0]);
    fprintf(f,"%s " PARS_DOUBLE_FORMAT "\n", ccDebugLabel("%s openloop_rev_act[1]", prefix), rst_pars->openloop_reverse.act[1]);

    fprintf(f,"%s " PARS_DOUBLE_FORMAT "\n", ccDebugLabel("%s t0_correction"      , prefix), rst_pars->t0_correction          );

    for(i = 0 ; i < REG_N_RST_COEFFS ; i++)
    {
        fprintf(f,"%s " PARS_DOUBLE_FORMAT " " PARS_DOUBLE_FORMAT " " PARS_DOUBLE_FORMAT " "
                        PARS_DOUBLE_FORMAT " " PARS_DOUBLE_FORMAT " " PARS_DOUBLE_FORMAT "\n", ccDebugLabel("%s R:S:T:A:B:AS+BR", prefix),
                        rst_pars->rst.r[i],
                        rst_pars->rst.s[i],
                        rst_pars->rst.t[i],
                        rst_pars->a[i],
                        rst_pars->b[i],
                        rst_pars->asbr[i]);
    }

    fputc('\n',f);
}
/*---------------------------------------------------------------------------------------------------------*/
void ccDebugPrint(FILE *f)
/*---------------------------------------------------------------------------------------------------------*/
{
    uint32_t i;
    uint32_t cyc_sel;

    // Report reference function meta data for all armed cycle selectors

    for(cyc_sel = 0 ; cyc_sel < CC_NUM_CYC_SELS; cyc_sel++)
    {
        if(ccrun.is_used[cyc_sel])
        {
            ccDebugFuncMeta(f, "REF", cyc_sel);
        }
    }

    // Report command parameters that are enabled

    if(ccpars_global.sim_load == REG_ENABLED)
    {
        // Report cycle log

        for(i = 0 ; i < ccrun.num_cycles ; i++)
        {
            fprintf(f,"%s " PARS_INT_FORMAT ":%s\n", ccDebugLabel("GLOBAL CYCLE_SELECTOR[%u]" , i),
                                                     ccpars_global.cycle_selector[i],
                                                     ccParsEnumString(enum_function_type, ccpars_ref.function[ccpars_global.cycle_selector[i]][0]));

            fprintf(f,"%s " PARS_DOUBLE_FORMAT "\n",   ccDebugLabel("ccrun.func[%u].ref_advance", i), ccrun.func[i].ref_advance);
            fprintf(f,"%s " PARS_FLOAT_FORMAT  "\n\n", ccDebugLabel("ccrun.func[%u].max_abs_err", i), ccrun.func[i].max_abs_err);
        }

        // Report load select and test_load select

        fprintf(f,"%s " PARS_INT_FORMAT "\n",  ccDebugLabel("%s select", "LOAD"), conv.par_values.load_select[0]);

        ccDebugPrintLoad(f, "LOAD", &conv.load_pars);

        if(ccrun.is_test_select_used)
        {
            fprintf(f,"%s " PARS_INT_FORMAT   "\n",  ccDebugLabel("TEST_LOAD", "select"), conv.par_values.load_test_select[0]);

            ccDebugPrintLoad(f, "TEST_LOAD", &conv.load_pars_test);
        }

        fprintf(f,"%s " PARS_INT_FORMAT   "\n",  ccDebugLabel("%s is_load_undersampled", "SIMLOAD"), conv.sim_load_pars.is_load_undersampled);
        fprintf(f,"%s " PARS_FLOAT_FORMAT "\n\n",ccDebugLabel("%s period_tc_ratio"     , "SIMLOAD"), conv.sim_load_pars.period_tc_ratio     );

        if(conv.sim_load_pars.tc_error != 0.0)
        {
            fprintf(f,"%s " PARS_FLOAT_FORMAT "\n",ccDebugLabel("%s tc_error", "SIMLOAD"), conv.sim_load_pars.tc_error);

            ccDebugPrintLoad(f, "SIMLOAD", &conv.sim_load_pars.load_pars);
        }

        // Report internally calculated voltage source parameters

        for(i = 0 ; i < REG_N_VS_SIM_COEFFS ; i++)
        {
            fprintf(f,"%s " PARS_DOUBLE_FORMAT " " PARS_DOUBLE_FORMAT "\n", ccDebugLabel( "%s num[%u]:den[%u]", "SIMVS", i, i),
                     conv.sim_vs_pars.num[i],
                     conv.sim_vs_pars.den[i]);
        }

        fprintf(f,"\n%s " PARS_INT_FORMAT   "\n",   ccDebugLabel("%s is_vs_undersampled", "SIMVS"), conv.sim_vs_pars.is_vs_undersampled);
        fprintf(f,"%s "   PARS_FLOAT_FORMAT "\n",   ccDebugLabel("%s vs_delay_iters"    , "SIMVS"), conv.sim_vs_pars.vs_delay_iters    );
        fprintf(f,"%s "   PARS_FLOAT_FORMAT "\n\n", ccDebugLabel("%s gain"              , "SIMVS"), conv.sim_vs_pars.gain              );

        // Report internally calculated field measurement filter and regulation parameters

        if(conv.b.regulation == REG_ENABLED)
        {
            ccDebugPrintMeas(f, "MEAS B", &conv.b.meas);

            ccDebugPrintReg(f, "BREG", &conv.b.last_op_rst_pars);
        }

        // Report current meas_filter parameters

        ccDebugPrintMeas(f, "MEAS I", &conv.i.meas);

        // Report internally calculated current regulation parameters

        if(conv.i.regulation == REG_ENABLED)
        {
            ccDebugPrintReg(f, "IREG", &conv.i.last_op_rst_pars);
        }
    }
}
// EOF

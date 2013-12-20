#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# Normal time tests

(
"$cctest" -o $output_format -g pars/global_10ms        -f PLEP   -d functions/plep                             > "$outpath/plep.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f PLEP   -d functions/plep1                            > "$outpath/plep1.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f PLEP   -d functions/plep2                            > "$outpath/plep2.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f PLEP   -d functions/plep3                            > "$outpath/plep3.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f PLEP   -d functions/plep4                            > "$outpath/plep4.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f PPPL   -d functions/pppl                             > "$outpath/pppl.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f SQUARE -d functions/test                             > "$outpath/square.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f STEPS  -d functions/test                             > "$outpath/steps.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f SINE   -d functions/test                             > "$outpath/sine.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f SINE   -d functions/test_window_enabled              > "$outpath/sine_window.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f COSINE -d functions/test                             > "$outpath/cosine.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f COSINE -d functions/test_window_enabled              > "$outpath/cosine_window.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f LTRIM  -d functions/trim_5s_duration                 > "$outpath/ltrim_5s_duration.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f CTRIM  -d functions/trim_5s_duration                 > "$outpath/ctrim_5s_duration.$file_type" &&
"$cctest" -o $output_format -g pars/global_with_limits -f LTRIM  -d functions/trim_no_duration -m pars/limits  > "$outpath/ltrim_no_duration.$file_type" &&
"$cctest" -o $output_format -g pars/global_with_limits -f CTRIM  -d functions/trim_no_duration -m pars/limits  > "$outpath/ctrim_no_duration.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f TABLE  -d functions/table1                           > "$outpath/table1.$file_type" &&
"$cctest" -o $output_format -g pars/global_100ms       -f TABLE  -d functions/table2 -m pars/limits_spline     > "$outpath/table2.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f TABLE  -d functions/table3                           > "$outpath/table3.$file_type" &&
"$cctest" -o $output_format -g pars/global_1s          -f TABLE  -d functions/table4                           > "$outpath/table4.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f TABLE  -d functions/table5                           > "$outpath/table5.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f TABLE  -d functions/table6                           > "$outpath/table6.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f SPLINE -d functions/table1                           > "$outpath/spline1.$file_type" &&
"$cctest" -o $output_format -g pars/global_100ms       -f SPLINE -d functions/table2 -m pars/limits_spline     > "$outpath/spline2.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f SPLINE -d functions/table3                           > "$outpath/spline3.$file_type" &&
"$cctest" -o $output_format -g pars/global_1s          -f SPLINE -d functions/table4                           > "$outpath/spline4.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f SPLINE -d functions/table5                           > "$outpath/spline5.$file_type" &&
"$cctest" -o $output_format -g pars/global_10ms        -f SPLINE -d functions/table6                           > "$outpath/spline6.$file_type"
) || exit 1

# Reverse time tests

(
"$cctest" -o $output_format -g pars/global_reverse_time -f TABLE  -d functions/table1 > "$outpath/reverse-table.$file_type"     &&
"$cctest" -o $output_format -g pars/global_reverse_time -f SPLINE -d functions/table1 > "$outpath/reverse-spline.$file_type"    &&
"$cctest" -o $output_format -g pars/global_reverse_time -f PLEP   -d functions/plep   > "$outpath/reverse-plep.$file_type"      &&
"$cctest" -o $output_format -g pars/global_reverse_time -f PPPL   -d functions/pppl   > "$outpath/reverse-pppl.$file_type"
) || exit 1

# EOF

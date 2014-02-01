#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

(
"$cctest" -o $output_format -g pars/amps/global  -f TABLE  -d functions/amps/table_d  -m pars/limits -l pars/load_d  -s pars/vs -r pars/amps/reg  > "$outpath/amps-table_d.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global  -f TABLE  -d functions/amps/table_qf -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-table_qf.$file_type" &&
"$cctest" -o $output_format -g pars/amps/global  -f TABLE  -d functions/amps/table_qd -m pars/limits -l pars/load_qd -s pars/vs -r pars/amps/reg  > "$outpath/amps-table_qd.$file_type" 
) || exit 1

# EOF

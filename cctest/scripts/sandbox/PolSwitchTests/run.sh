#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# LOAD.POL_SWI_AUTO DISABLED

#     LIMITS.INVERT_LIMITS DISABLED

"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/table     -m pars/limits_normal -l pars/load_pol_swi_auto_disabled -r pars/amps/reg -s pars/vs > "$outpath/amps-table-01.$file_type"   
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/table_pos -m pars/limits_normal -l pars/load_pol_swi_auto_disabled -r pars/amps/reg -s pars/vs > "$outpath/amps-table-02.$file_type"   
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/table_neg -m pars/limits_normal -l pars/load_pol_swi_auto_disabled -r pars/amps/reg -s pars/vs > "$outpath/amps-table-03.$file_type"   

#     LIMITS.INVERT_LIMITS ENABLED

"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/table     -m pars/limits_inverted -l pars/load_pol_swi_auto_disabled -r pars/amps/reg -s pars/vs > "$outpath/amps-table-04.$file_type"   
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/table_pos -m pars/limits_inverted -l pars/load_pol_swi_auto_disabled -r pars/amps/reg -s pars/vs > "$outpath/amps-table-05.$file_type"   
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/table_neg -m pars/limits_inverted -l pars/load_pol_swi_auto_disabled -r pars/amps/reg -s pars/vs > "$outpath/amps-table-06.$file_type"   

# LOAD.POL_SWI_AUTO ENABLED

#     LIMITS.INVERT_LIMITS DISABLED

"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/table     -m pars/limits_normal -l pars/load_pol_swi_auto_enabled -r pars/amps/reg -s pars/vs > "$outpath/amps-table-07.$file_type"   
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/table_pos -m pars/limits_normal -l pars/load_pol_swi_auto_enabled -r pars/amps/reg -s pars/vs > "$outpath/amps-table-08.$file_type"   
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/table_neg -m pars/limits_normal -l pars/load_pol_swi_auto_enabled -r pars/amps/reg -s pars/vs > "$outpath/amps-table-09.$file_type"   

#     LIMITS.INVERT_LIMITS ENABLED

"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/table     -m pars/limits_inverted -l pars/load_pol_swi_auto_enabled -r pars/amps/reg -s pars/vs > "$outpath/amps-table-10.$file_type"   
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/table_pos -m pars/limits_inverted -l pars/load_pol_swi_auto_enabled -r pars/amps/reg -s pars/vs > "$outpath/amps-table-11.$file_type"   
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/table_neg -m pars/limits_inverted -l pars/load_pol_swi_auto_enabled -r pars/amps/reg -s pars/vs > "$outpath/amps-table-12.$file_type"   

# EOF

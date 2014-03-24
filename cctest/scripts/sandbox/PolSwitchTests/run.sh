#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# LOAD.POL_SWI_AUTO DISABLED

#     LIMITS.INVERT_LIMITS DISABLED

cat pars/amps/global functions/amps/table     pars/limits_normal   pars/load_pol_swi_auto_disabled pars/meas pars/amps/reg pars/vs | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION TABLE" > "$outpath/amps-table-01.$file_type"   
cat pars/amps/global functions/amps/table_pos pars/limits_normal   pars/load_pol_swi_auto_disabled pars/meas pars/amps/reg pars/vs | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION TABLE" > "$outpath/amps-table-02.$file_type"   
cat pars/amps/global functions/amps/table_neg pars/limits_normal   pars/load_pol_swi_auto_disabled pars/meas pars/amps/reg pars/vs | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION TABLE" > "$outpath/amps-table-03.$file_type"   

#     LIMITS.INVERT_LIMITS ENABLED

cat pars/amps/global functions/amps/table     pars/limits_inverted pars/load_pol_swi_auto_disabled pars/meas pars/amps/reg pars/vs | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION TABLE" > "$outpath/amps-table-04.$file_type"   
cat pars/amps/global functions/amps/table_pos pars/limits_inverted pars/load_pol_swi_auto_disabled pars/meas pars/amps/reg pars/vs | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION TABLE" > "$outpath/amps-table-05.$file_type"   
cat pars/amps/global functions/amps/table_neg pars/limits_inverted pars/load_pol_swi_auto_disabled pars/meas pars/amps/reg pars/vs | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION TABLE" > "$outpath/amps-table-06.$file_type"   

# LOAD.POL_SWI_AUTO ENABLED

#     LIMITS.INVERT_LIMITS DISABLED

cat pars/amps/global functions/amps/table     pars/limits_normal   pars/load_pol_swi_auto_enabled  pars/meas pars/amps/reg pars/vs | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION TABLE" > "$outpath/amps-table-07.$file_type"   
cat pars/amps/global functions/amps/table_pos pars/limits_normal   pars/load_pol_swi_auto_enabled  pars/meas pars/amps/reg pars/vs | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION TABLE" > "$outpath/amps-table-08.$file_type"   
cat pars/amps/global functions/amps/table_neg pars/limits_normal   pars/load_pol_swi_auto_enabled  pars/meas pars/amps/reg pars/vs | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION TABLE" > "$outpath/amps-table-09.$file_type"   

#     LIMITS.INVERT_LIMITS ENABLED

cat pars/amps/global functions/amps/table     pars/limits_inverted pars/load_pol_swi_auto_enabled  pars/meas pars/amps/reg pars/vs | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION TABLE" > "$outpath/amps-table-10.$file_type"   
cat pars/amps/global functions/amps/table_pos pars/limits_inverted pars/load_pol_swi_auto_enabled  pars/meas pars/amps/reg pars/vs | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION TABLE" > "$outpath/amps-table-11.$file_type"   
cat pars/amps/global functions/amps/table_neg pars/limits_inverted pars/load_pol_swi_auto_enabled  pars/meas pars/amps/reg pars/vs | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION TABLE" > "$outpath/amps-table-12.$file_type"   

# EOF

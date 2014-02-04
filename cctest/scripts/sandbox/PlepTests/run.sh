#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# PLEP tests

"$cctest" -o $output_format -g pars/global_10ms -f PLEP -d functions/plep01  > "$outpath/plep01.$file_type" 
"$cctest" -o $output_format -g pars/global_10ms -f PLEP -d functions/plep02  > "$outpath/plep02.$file_type" 
"$cctest" -o $output_format -g pars/global_10ms -f PLEP -d functions/plep03  > "$outpath/plep03.$file_type" 
"$cctest" -o $output_format -g pars/global_10ms -f PLEP -d functions/plep04  > "$outpath/plep04.$file_type" 
"$cctest" -o $output_format -g pars/global_10ms -f PLEP -d functions/plep05  > "$outpath/plep05.$file_type" 

# EOF

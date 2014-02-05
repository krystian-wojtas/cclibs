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
"$cctest" -o $output_format -g pars/global_10ms -f PLEP -d functions/plep06  > "$outpath/plep06.$file_type" 
"$cctest" -o $output_format -g pars/global_10ms -f PLEP -d functions/plep07  > "$outpath/plep07.$file_type" 
"$cctest" -o $output_format -g pars/global_10ms -f PLEP -d functions/plep08  > "$outpath/plep08.$file_type" 
"$cctest" -o $output_format -g pars/global_10ms -f PLEP -d functions/plep09  > "$outpath/plep09.$file_type" 
"$cctest" -o $output_format -g pars/global_10ms -f PLEP -d functions/plep10  > "$outpath/plep10.$file_type" 
"$cctest" -o $output_format -g pars/global_10ms -f PLEP -d functions/plep11  > "$outpath/plep11.$file_type" 
"$cctest" -o $output_format -g pars/global_10ms -f PLEP -d functions/plep12  > "$outpath/plep12.$file_type" 

# EOF

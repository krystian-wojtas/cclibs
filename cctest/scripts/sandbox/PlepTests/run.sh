#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# PLEP tests

cat pars/global_10ms functions/plep01 | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION PLEP" > "$outpath/plep01.$file_type" 
cat pars/global_10ms functions/plep02 | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION PLEP" > "$outpath/plep02.$file_type" 
cat pars/global_10ms functions/plep03 | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION PLEP" > "$outpath/plep03.$file_type" 
cat pars/global_10ms functions/plep04 | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION PLEP" > "$outpath/plep04.$file_type" 
cat pars/global_10ms functions/plep05 | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION PLEP" > "$outpath/plep05.$file_type" 
cat pars/global_10ms functions/plep06 | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION PLEP" > "$outpath/plep06.$file_type" 
cat pars/global_10ms functions/plep07 | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION PLEP" > "$outpath/plep07.$file_type" 
cat pars/global_10ms functions/plep08 | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION PLEP" > "$outpath/plep08.$file_type" 
cat pars/global_10ms functions/plep09 | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION PLEP" > "$outpath/plep09.$file_type" 
cat pars/global_10ms functions/plep10 | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION PLEP" > "$outpath/plep10.$file_type" 
cat pars/global_10ms functions/plep11 | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION PLEP" > "$outpath/plep11.$file_type" 
cat pars/global_10ms functions/plep12 | "$cctest" "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION PLEP" > "$outpath/plep12.$file_type" 

# EOF

#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

(
"$cctest" -o $output_format -g pars/volts/global -f TABLE  -d functions/volts/table -m pars/limits -l pars/load -s pars/vs                   > "$outpath/volts-table.$file_type" &&
"$cctest" -o $output_format -g pars/volts/global -f PLEP   -d functions/volts/plep  -m pars/limits -l pars/load -s pars/vs                   > "$outpath/volts-plep.$file_type" &&
"$cctest" -o $output_format -g pars/volts/global -f PPPL   -d functions/volts/pppl  -m pars/limits -l pars/load -s pars/vs                   > "$outpath/volts-pppl.$file_type" &&
"$cctest" -o $output_format -g pars/amps/global  -f TABLE  -d functions/amps/table  -m pars/limits -l pars/load -s pars/vs -r pars/amps/reg  > "$outpath/amps-table.$file_type" &&
"$cctest" -o $output_format -g pars/amps/global  -f PPPL   -d functions/amps/pppl   -m pars/limits -l pars/load -s pars/vs -r pars/amps/reg  > "$outpath/amps-pppl.$file_type" &&
"$cctest" -o $output_format -g pars/amps/global  -f START  -d functions/amps/start  -m pars/limits -l pars/load -s pars/vs -r pars/amps/reg  > "$outpath/amps-start.$file_type" &&
"$cctest" -o $output_format -g pars/gauss/global -f TABLE  -d functions/gauss/table -m pars/limits -l pars/load -s pars/vs -r pars/gauss/reg > "$outpath/gauss-table.$file_type" &&
"$cctest" -o $output_format -g pars/gauss/global -f PPPL   -d functions/gauss/pppl  -m pars/limits -l pars/load -s pars/vs -r pars/gauss/reg > "$outpath/gauss-pppl.$file_type" &&
"$cctest" -o $output_format -g pars/gauss/global -f START  -d functions/gauss/start -m pars/limits -l pars/load -s pars/vs -r pars/gauss/reg > "$outpath/gauss-start.$file_type" 
) || exit 1

# EOF

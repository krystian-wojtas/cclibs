#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

(
"$cctest" -o $output_format -g pars/amps/global  -f TABLE   -d functions/amps/table -m pars/limits -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-table.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global  -f PPPL    -d functions/amps/pppl  -m pars/limits -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-pppl.$file_type"   &&
"$cctest" -o $output_format -g pars/gauss/global -f PPPL    -d functions/gauss/pppl -m pars/limits -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/gauss-pppl.$file_type"
) || exit 1

# EOF

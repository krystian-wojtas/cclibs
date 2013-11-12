#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

(
"$cctest" -o $output_format -g pars/amps/global -f SPLINE  -d functions/amps/table -m pars/limits -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-spline.$file_type" &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE  -d functions/amps/table -m pars/limits -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-table.$file_type"
) || exit 1

# EOF

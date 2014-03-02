#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

(
"$cctest" -o $output_format -g pars/amps/global -f RAMP  -d functions/amps/ramp -m pars/limits -l pars/load -s pars/vs -r pars/amps/reg  > "$outpath/ramp.$file_type" 
) || exit 1

# EOF

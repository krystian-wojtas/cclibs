#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

(
"$cctest" -o $output_format -g pars/amps/global -f PLEP   -d functions/amps/plep  -m pars/limits -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-plep.$file_type"
) || exit 1

# EOF

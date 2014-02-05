#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

(
"$cctest" -o $output_format -g pars/amps/global -f PPPL -d functions/amps/pppl_mb -m pars/limits -l pars/load_mb -s pars/vs -r pars/amps/reg  > "$outpath/amps-pppl_mb.$file_type"  
) || exit 1

# EOF

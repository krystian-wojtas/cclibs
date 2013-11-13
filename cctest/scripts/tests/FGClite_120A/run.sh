#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

(
"$cctest" -o $output_format -g pars/volts/global -f COSINE -d ../FGC2_60A/functions/volts/test -m pars/limits -l pars/load                    -s pars/vs > "$outpath/volts-cosine.$file_type"     &&
"$cctest" -o $output_format -g pars/amps/global  -f TABLE  -d ../FGC2_60A/functions/amps/table -m pars/limits -l pars/load -r pars/amps/reg80 -s pars/vs > "$outpath/amps-table-80ms.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global  -f SPLINE -d ../FGC2_60A/functions/amps/table -m pars/limits -l pars/load -r pars/amps/reg80 -s pars/vs > "$outpath/amps-spline-80ms.$file_type" &&
"$cctest" -o $output_format -g pars/amps/global  -f PLEP   -d ../FGC2_60A/functions/amps/plep  -m pars/limits -l pars/load -r pars/amps/reg80 -s pars/vs > "$outpath/amps-plep-80ms.$file_type"   &&
"$cctest" -o $output_format -g pars/amps/global  -f PLEP   -d ../FGC2_60A/functions/amps/plep  -m pars/limits -l pars/load -r pars/amps/reg40 -s pars/vs > "$outpath/amps-plep-40ms.$file_type"   &&
"$cctest" -o $output_format -g pars/amps/global  -f PLEP   -d ../FGC2_60A/functions/amps/plep  -m pars/limits -l pars/load -r pars/amps/reg20 -s pars/vs > "$outpath/amps-plep-20ms.$file_type"
) || exit 1

# EOF

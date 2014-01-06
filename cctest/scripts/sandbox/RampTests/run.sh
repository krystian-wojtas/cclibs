#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# RAMP tests

(
"$cctest" -o $output_format -g pars/volts/global -f RAMP -d functions/volts/ramp_up   -m pars/volts/limits_high -l pars/load                  -s pars/vs > "$outpath/volts-ramp_up_high.$file_type"   &&
"$cctest" -o $output_format -g pars/volts/global -f RAMP -d functions/volts/ramp_up   -m pars/volts/limits_low  -l pars/load                  -s pars/vs > "$outpath/volts-ramp_up_low.$file_type"    &&
"$cctest" -o $output_format -g pars/volts/global -f RAMP -d functions/volts/ramp_down -m pars/volts/limits_high -l pars/load                  -s pars/vs > "$outpath/volts-ramp_down_high.$file_type" &&
"$cctest" -o $output_format -g pars/volts/global -f RAMP -d functions/volts/ramp_down -m pars/volts/limits_low  -l pars/load                  -s pars/vs > "$outpath/volts-ramp_down_low.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global  -f RAMP -d functions/amps/ramp_up    -m pars/amps/limits_high -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-ramp_up_high.$file_type"    &&
"$cctest" -o $output_format -g pars/amps/global  -f RAMP -d functions/amps/ramp_up    -m pars/amps/limits_low  -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-ramp_up_low.$file_type"     &&
"$cctest" -o $output_format -g pars/amps/global  -f RAMP -d functions/amps/ramp_down  -m pars/amps/limits_high -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-ramp_down_high.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global  -f RAMP -d functions/amps/ramp_down  -m pars/amps/limits_low  -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-ramp_down_low.$file_type" 
) || exit 1

# RAMP abort tests

(
"$cctest" -o $output_format -g pars/amps/global_abort1  -f RAMP -d functions/amps/ramp_up    -m pars/amps/limits_abort -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-ramp_up_abort1.$file_type"    &&
"$cctest" -o $output_format -g pars/amps/global_abort1  -f RAMP -d functions/amps/ramp_down  -m pars/amps/limits_abort -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-ramp_down_abort1.$file_type"  
) || exit 1

# EOF

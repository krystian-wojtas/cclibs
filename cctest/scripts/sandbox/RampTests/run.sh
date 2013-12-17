#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# RAMP tests

(
"$cctest" -o $output_format -g pars/volts/global -f RAMP -d functions/volts/ramp_up   -m pars/limits_v_high -l pars/load                  -s pars/vs > "$outpath/volts-ramp_up_high.$file_type"   &&
"$cctest" -o $output_format -g pars/volts/global -f RAMP -d functions/volts/ramp_up   -m pars/limits_v_low  -l pars/load                  -s pars/vs > "$outpath/volts-ramp_up_low.$file_type"    &&
"$cctest" -o $output_format -g pars/volts/global -f RAMP -d functions/volts/ramp_down -m pars/limits_v_high -l pars/load                  -s pars/vs > "$outpath/volts-ramp_down_high.$file_type" &&
"$cctest" -o $output_format -g pars/volts/global -f RAMP -d functions/volts/ramp_down -m pars/limits_v_low  -l pars/load                  -s pars/vs > "$outpath/volts-ramp_down_low.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global  -f RAMP -d functions/amps/ramp_up    -m pars/limits_i_high -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-ramp_up_high.$file_type"    &&
"$cctest" -o $output_format -g pars/amps/global  -f RAMP -d functions/amps/ramp_up    -m pars/limits_i_low  -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-ramp_up_low.$file_type"     &&
"$cctest" -o $output_format -g pars/amps/global  -f RAMP -d functions/amps/ramp_down  -m pars/limits_i_high -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-ramp_down_high.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global  -f RAMP -d functions/amps/ramp_down  -m pars/limits_i_low  -l pars/load -r pars/amps/reg -s pars/vs > "$outpath/amps-ramp_down_low.$file_type" 
) || exit 1

# EOF

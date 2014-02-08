#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

(
"$cctest" -o $output_format -g pars/amps/global -f PPPL  -d functions/amps/pppl_mb            -m pars/limits -l pars/load_mb -s pars/vs -r pars/amps/reg  > "$outpath/amps-pppl_mb.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/LHCFAST2_mb        -m pars/limits -l pars/load_mb -s pars/vs -r pars/amps/reg  > "$outpath/amps-LHCFAST2_mb.$file_type" && 
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/CNGS_400GeV_mb     -m pars/limits -l pars/load_mb -s pars/vs -r pars/amps/reg  > "$outpath/amps-CNGS_400GeV_mb.$file_type" && 
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/LHC_4INJ_450Gev_mb -m pars/limits -l pars/load_mb -s pars/vs -r pars/amps/reg  > "$outpath/amps-LHC_4INJ_450Gev_mb.$file_type" && 
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/SFTLONG_400Gev_mb  -m pars/limits -l pars/load_mb -s pars/vs -r pars/amps/reg  > "$outpath/amps-SFTLONG_400Gev_mb.$file_type" && 
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/CNGS-measured_mb   -m pars/limits -l pars/load_mb -s pars/vs -r pars/amps/reg  > "$outpath/amps-CNGS-measured_mb.$file_type"  
) || exit 1

# EOF

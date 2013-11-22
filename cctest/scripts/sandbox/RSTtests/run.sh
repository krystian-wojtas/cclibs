#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

(
"$cctest" -o $output_format -g pars/global-amps  -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load1     -r pars/reg/reg1     -s pars/vs > "$outpath/amps-pppl1.$file_type"      &&
"$cctest" -o $output_format -g pars/global-amps  -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load1     -r pars/reg/reg1     -s pars/vs > "$outpath/amps-sine1.$file_type"      &&
"$cctest" -o $output_format -g pars/global-amps  -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load2     -r pars/reg/reg2     -s pars/vs > "$outpath/amps-pppl2.$file_type"      &&
"$cctest" -o $output_format -g pars/global-amps  -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load2     -r pars/reg/reg2     -s pars/vs > "$outpath/amps-sine2.$file_type"      &&
"$cctest" -o $output_format -g pars/global-amps  -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load2-dec -r pars/reg/reg2-dec -s pars/vs > "$outpath/amps-pppl2-dec.$file_type"  &&
"$cctest" -o $output_format -g pars/global-amps  -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load2-dec -r pars/reg/reg2-dec -s pars/vs > "$outpath/amps-sine2-dec.$file_type"  &&
"$cctest" -o $output_format -g pars/global-amps  -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load3     -r pars/reg/reg3     -s pars/vs > "$outpath/amps-pppl3.$file_type"      &&
"$cctest" -o $output_format -g pars/global-amps  -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load3     -r pars/reg/reg3     -s pars/vs > "$outpath/amps-sine3.$file_type"      &&
"$cctest" -o $output_format -g pars/global-amps  -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load3-dec -r pars/reg/reg3-dec -s pars/vs > "$outpath/amps-pppl3-dec.$file_type"  &&
"$cctest" -o $output_format -g pars/global-amps  -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load3-dec -r pars/reg/reg3-dec -s pars/vs > "$outpath/amps-sine3-dec.$file_type"  &&
"$cctest" -o $output_format -g pars/global-amps  -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load4     -r pars/reg/reg4     -s pars/vs > "$outpath/amps-pppl4.$file_type"      &&
"$cctest" -o $output_format -g pars/global-amps  -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load4     -r pars/reg/reg4     -s pars/vs > "$outpath/amps-sine4.$file_type"      &&
"$cctest" -o $output_format -g pars/global-amps  -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load4-dec -r pars/reg/reg4-dec -s pars/vs > "$outpath/amps-pppl4-dec.$file_type"  &&
"$cctest" -o $output_format -g pars/global-amps  -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load4-dec -r pars/reg/reg4-dec -s pars/vs > "$outpath/amps-sine4-dec.$file_type"  &&
"$cctest" -o $output_format -g pars/global-gauss -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load1     -r pars/reg/reg1     -s pars/vs > "$outpath/gauss-pppl1.$file_type"     &&
"$cctest" -o $output_format -g pars/global-gauss -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load1     -r pars/reg/reg1     -s pars/vs > "$outpath/gauss-sine1.$file_type"     &&
"$cctest" -o $output_format -g pars/global-gauss -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load2     -r pars/reg/reg2     -s pars/vs > "$outpath/gauss-pppl2.$file_type"     &&
"$cctest" -o $output_format -g pars/global-gauss -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load2     -r pars/reg/reg2     -s pars/vs > "$outpath/gauss-sine2.$file_type"     &&
"$cctest" -o $output_format -g pars/global-gauss -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load2-dec -r pars/reg/reg2-dec -s pars/vs > "$outpath/gauss-pppl2-dec.$file_type" &&
"$cctest" -o $output_format -g pars/global-gauss -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load2-dec -r pars/reg/reg2-dec -s pars/vs > "$outpath/gauss-sine2-dec.$file_type" &&
"$cctest" -o $output_format -g pars/global-gauss -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load3     -r pars/reg/reg3     -s pars/vs > "$outpath/gauss-pppl3.$file_type"     &&
"$cctest" -o $output_format -g pars/global-gauss -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load3     -r pars/reg/reg3     -s pars/vs > "$outpath/gauss-sine3.$file_type"     &&
"$cctest" -o $output_format -g pars/global-gauss -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load3-dec -r pars/reg/reg3-dec -s pars/vs > "$outpath/gauss-pppl3-dec.$file_type" &&
"$cctest" -o $output_format -g pars/global-gauss -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load3-dec -r pars/reg/reg3-dec -s pars/vs > "$outpath/gauss-sine3-dec.$file_type" &&
"$cctest" -o $output_format -g pars/global-gauss -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load4     -r pars/reg/reg4     -s pars/vs > "$outpath/gauss-pppl4.$file_type"     &&
"$cctest" -o $output_format -g pars/global-gauss -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load4     -r pars/reg/reg4     -s pars/vs > "$outpath/gauss-sine4.$file_type"     &&
"$cctest" -o $output_format -g pars/global-gauss -f PPPL    -d functions/pppl                 -m pars/limits -l pars/load/load4-dec -r pars/reg/reg4-dec -s pars/vs > "$outpath/gauss-pppl4-dec.$file_type" &&
"$cctest" -o $output_format -g pars/global-gauss -f SINE    -d functions/test_window_enabled  -m pars/limits -l pars/load/load4-dec -r pars/reg/reg4-dec -s pars/vs > "$outpath/gauss-sine4-dec.$file_type" 
) || exit 1

# EOF

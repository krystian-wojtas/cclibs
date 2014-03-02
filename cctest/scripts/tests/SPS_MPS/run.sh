#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

(
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/CNGS_2009_AFTER_FT -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-CNGS_2009_AFTER_FT.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/Hiradmat_High_Intensity_2011 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-HIRADMAT_HIGH_INTENSITY_2011.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/LHC_25ns_Q20_2012 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-LHC_25NS_Q20_2012.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/LHC_50ns_D_2011 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-LHC_50NS_D_2011.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/LHC_INDIV_Q20_2012 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-LHC_INDIV_Q20_2012.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/LHC_ION_12Inj_Q20_2012 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-LHC_ION_12INJ_Q20_2012.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/LHC_ION_1Inj_Q20_2012 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-LHC_ION_1INJ_Q20_2012.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/LHC_PILOT_2011 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-LHC_PILOT_2011.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/LHC_PILOT_Q20_2012 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-LHC_PILOT_Q20_2012.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/MD_26_120_18000_2009 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-MD_26_120_18000_2009.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/MD_26_L7200_Q20 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-MD_26_L7200_Q20.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/SFT_ION_2Inj_Debunching_200GeV_2012 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-SFT_ION_2INJ_DEBUNCHING_200GEV_2012.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/SFT_LONG_L4890_2013 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-SFT_LONG_L4890_2013.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/QF/SFT_LONG_L9690_2010 -m pars/limits -l pars/load_qf -s pars/vs -r pars/amps/reg  > "$outpath/amps-QF-SFT_LONG_L9690_2010.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/CNGS_2009_AFTER_FT -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-CNGS_2009_AFTER_FT.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/Hiradmat_High_Intensity_2011 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-HIRADMAT_HIGH_INTENSITY_2011.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/LHC_25ns_Q20_2012 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-LHC_25NS_Q20_2012.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/LHC_50ns_D_2011 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-LHC_50NS_D_2011.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/LHC_INDIV_Q20_2012 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-LHC_INDIV_Q20_2012.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/LHC_ION_12Inj_Q20_2012 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-LHC_ION_12INJ_Q20_2012.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/LHC_ION_1Inj_Q20_2012 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-LHC_ION_1INJ_Q20_2012.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/LHC_PILOT_2011 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-LHC_PILOT_2011.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/LHC_PILOT_Q20_2012 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-LHC_PILOT_Q20_2012.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/MD_26_120_18000_2009 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-MD_26_120_18000_2009.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/MD_26_L7200_Q20 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-MD_26_L7200_Q20.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/SFT_ION_2Inj_Debunching_200GeV_2012 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-SFT_ION_2INJ_DEBUNCHING_200GEV_2012.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/SFT_LONG_L4890_2013 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-SFT_LONG_L4890_2013.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/RB/SFT_LONG_L9690_2010 -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-RB-SFT_LONG_L9690_2010.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f PPPL  -d functions/amps/pppl_mb            -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-pppl_mb.$file_type"  &&
"$cctest" -o $output_format -g pars/amps/global -f TABLE -d functions/amps/CNGS-measured_mb   -m pars/limits -l pars/load_rb -s pars/vs -r pars/amps/reg  > "$outpath/amps-CNGS-measured_mb.$file_type"  
) || exit 1

# EOF

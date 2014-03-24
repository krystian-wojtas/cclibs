#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# Normal time tests

(
cat pars/global functions/test | "$cctest"  "GLOBAL.OUTPUT_FORMAT $output_format" "GLOBAL.FUNCTION SINE" > "$outpath/sine.$file_type" 
) || exit 1


# EOF

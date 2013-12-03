#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# Normal time tests

(
"$cctest" -o $output_format -g pars/global        -f SINE   -d functions/test                             > "$outpath/sine.$file_type" 
) || exit 1


# EOF

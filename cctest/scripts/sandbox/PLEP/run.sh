#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# PLEP tests

$cctest "global csv_format $csv_format" "read pleps.cct"

>&2 echo $0 complete

# EOF

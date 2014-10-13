#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# FGC2 60A tests

$cctest "global csv_format $csv_format" "read 60A.cct"

# EOF

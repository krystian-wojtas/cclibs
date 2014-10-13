#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# PSB2GEV tests

$cctest "global csv_format $csv_format" "read psb.cct"

# EOF

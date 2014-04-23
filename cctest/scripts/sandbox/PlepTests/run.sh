#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# PLEP tests

$cctest "g csv $csv_format" "read pleps.cct"

# EOF

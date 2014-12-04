#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# HRS tests

$cctest "global csv_format $csv_format" "read hrs.cct"

>&2 echo $0 complete

# EOF

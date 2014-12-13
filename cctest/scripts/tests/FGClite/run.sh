#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# FGClite tests

$cctest "global csv_format $csv_format" "read lite1000.cct"
$cctest "global csv_format $csv_format" "read lite100.cct"
$cctest "global csv_format $csv_format" "read lite50.cct"

>&2 echo $0 complete

# EOF

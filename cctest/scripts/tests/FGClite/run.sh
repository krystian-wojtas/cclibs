#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# FGClite tests

$cctest "global csv_format $csv_format" "read lite.cct"

>&2 echo $0 complete

# EOF

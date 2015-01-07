#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# GPS tests

$cctest "global csv_format $csv_format" "read gps.cct"

>&2 echo $0 complete

# EOF

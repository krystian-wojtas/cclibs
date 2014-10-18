#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# RST tests

$cctest "global csv_format $csv_format" "read ramp.cct"

>&2 echo $0 complete

# EOF

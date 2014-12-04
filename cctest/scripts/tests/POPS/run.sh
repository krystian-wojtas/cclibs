#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# POPS tests

$cctest "global csv_format $csv_format" "read pops.cct"

>&2 echo $0 complete

# EOF

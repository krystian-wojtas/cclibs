#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# RB tests

$cctest "global csv_format $csv_format" "read rb.cct"

>&2 echo $0 complete

# EOF

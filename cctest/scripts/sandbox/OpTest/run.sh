#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# Operation and test parameter switching tests

$cctest "global csv_format $csv_format" "read optest.cct"

>&2 echo $0 complete

# EOF

#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# RST tests

$cctest "global csv_format $csv_format" "read rst.cct"
$cctest "global csv_format $csv_format" "read rst-no-noise.cct"

# EOF

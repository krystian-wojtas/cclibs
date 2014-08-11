#!/bin/bash
#
cd `dirname $0`

source ../../run_header.sh

# CURRENT_REF actuation tests

$cctest "global csv_format $csv_format" "read iref.cct"

# EOF

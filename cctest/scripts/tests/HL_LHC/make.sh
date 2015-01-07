#!/bin/bash

cd `dirname $0`

source ../../run_header.sh

set +x

rm -rf cctest results
mkdir cctest results

dos2unix -q input/*

awk -f squeeze.awk input/*.csv

# Run CCTEST to simulate squeeze functions

echo -n "Starting cctest in 5s..."
sleep 1
echo -n "4..."
sleep 1
echo -n "3..."
sleep 1
echo -n "2..."
sleep 1
echo -n "1..."
sleep 1

$cctest "global csv_format $csv_format" "read squeeze.cct"

>&2 echo $0 complete

# EOF

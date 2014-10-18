#!/bin/bash
#
# Run all scripts

cd `dirname $0`

for script in */run.sh
do
    "$script" $1 > $script.log &
done

# EOF

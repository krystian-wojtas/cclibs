#!/bin/sh

for makefile in `find . -type f -name Makefile`
do
    cd `dirname $makefile`
    pwd
    make clean all || exit 1
    cd -
done

# EOF

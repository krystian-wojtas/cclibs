#!/bin/sh

for makefile in `find . -type f -maxdepth 2 -name Makefile`
do
    cd `dirname $makefile`
    pwd
    make clean all || exit 1
    cd -
done

# EOF

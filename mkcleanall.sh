#!/bin/sh

# make libs

for makefile in `find lib* -maxdepth 1 -type f -name Makefile`
do
    cd `dirname $makefile`
    echo -e "\n\n##################################################"
    pwd
    echo -e "##################################################\n\n"
    make clean all || exit 1
    cd -
done

# make test programs

for makefile in `find *test ccrt -maxdepth 1 -type f -name Makefile`
do
    cd `dirname $makefile`
    echo -e "\n\n##################################################"
    pwd
    echo -e "##################################################\n\n"
    make clean all || exit 1
    cd -
done

# EOF

#!/bin/sh

cd ~/projects/fgc/sw/lib/cclibs/

for makefile in `find . -maxdepth 2 -type f -name Makefile`
do
    cd `dirname $makefile`
    pwd
    make clean || exit 1
    cd -
done

cd ~/projects/cclibs/

rsync -av \
--exclude 'Linux/' \
--exclude 'results/' \
--exclude 'webplots/' \
--exclude 'resources/' \
--exclude 'html/' \
--exclude 'latex/' \
--exclude 'images/' \
--exclude 'man/' \
--exclude '.setting/' \
--exclude '.cctest*/' \
--exclude 'Makefile*' \
--exclude '.cproject' \
--include '*/' \
--include '*' \
~/projects/fgc/sw/lib/cclibs/ ~/projects/cclibs/

# EOF

#
# Script used in all run.sh scripts
#
if [ $# -eq 0 ]; then
  csv_format=NONE
elif [ $# -eq 1 ]; then
  csv_format=$1
else
  echo usage: run.sh [CSV_FORMAT]
  exit -1
fi

cctest=../../../`uname -s`/`uname -m`/cctest

set -x
# EOF

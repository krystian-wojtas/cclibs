#
# Script used in all run.sh scripts
#
if [ $# -eq 0 ]; then
  output_format=FLOT
elif [ $# -eq 1 ]; then
  output_format=$1
else
  echo usage: run.sh [OUTPUT_FORMAT]
  exit -1
fi

if [ $output_format = FLOT ]; then
  output_dir=webplots
  file_type=html
else
  output_dir=csv
  file_type=csv
fi

cctest=../../../`uname -s`/`uname -m`/cctest

dir=`pwd`
dirpath=`dirname "$dir"`
group=`basename "$dirpath"`
project=`basename "$dir"`
results_path=../../../results
outpath="$results_path/$output_dir/$group/$project"
rm -f "$outpath"/*.$file_type
mkdir -p "$outpath"

echo CCTEST - $output_format - $group - $project - $outpath
set -x
# EOF

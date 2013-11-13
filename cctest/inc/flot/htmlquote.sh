#!/bin/sh

# Check that a valid number of arguments have been supplied

if [ $# -lt 3 ]; then
    # Print usage and exit

    echo "Usage: $0 <header file> <footer file> <html file> [<html file 2>...<html file n>]" >&2
    exit 1
fi

# Extract header and footer arguments

header=$1
footer=$2
shift 2

# Print header

cat "$header"

# Quote the content of each HTML file

for html_file in $*
do
    # Escape quotes, quote each line and add a new line

    sed 's/"/\\"/g;s/^\(.*\)$/\"\1\\n\"/' "$html_file"

    # Delimit array elements

    echo ,
done

# Print footer

cat "$footer"

# EOF

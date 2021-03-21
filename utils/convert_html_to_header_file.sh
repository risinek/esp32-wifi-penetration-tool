#!/bin/bash
# Set PAGE_HEADER_LOC env variable if you want this script to also automatically add newly created header file to webserver
# Example: export PAGE_HEADER_LOC=../components/webserver/pages

if [ -z "$1" ] ; then
    echo "Missing argument - filename!"
    exit 1
fi

output_name=$(echo "$1" | sed 's/.html//')
output_filename="page_${output_name}.h"

echo """#ifndef PAGE_${output_name^^}_H
#define PAGE_${output_name^^}_H

const char page_${output_name,,}[] =""" > $output_filename


cat $1 | sed 's/http:\/\/192.168.4.1\///' | sed 's/"/\\"/g' | sed 's/ */&"/' | sed 's/$/\\n"&/' >> $output_filename

echo """
;
#endif
""" >> $output_filename

if [ -n "$PAGE_HEADER_LOC" ]; then
    echo "Copying $output_filename to $PAGE_HEADER_LOC/$output_filename"
    cp $output_filename $PAGE_HEADER_LOC/$output_filename
fi
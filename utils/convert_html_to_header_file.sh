#!/bin/bash

if [ -z "$1" ] ; then
    echo "Missing argument - filename!"
    exit 1
fi

output_folder="converted/"

output_name=$(echo "$1" | sed 's/.html//')
output_filename="page_${output_name}.h"
output_filename="${output_folder}${output_filename}"

echo """#ifndef PAGE_${output_name^^}_H
#define PAGE_${output_name^^}_H

const char page_${output_name,,}[] =""" > $output_filename


cat $1 | sed 's/"/\\"/g' | sed 's/ */&"/' | sed 's/$/"&/' >> $output_filename

echo """
;
#endif
""" >> $output_filename
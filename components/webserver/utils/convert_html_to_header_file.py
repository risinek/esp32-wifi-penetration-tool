#!/usr/bin/env python3

import sys, shutil, os, gzip

def bytes_to_c_array(data):
    return [format(b, '#04X') for b in data]

if (len(sys.argv) < 4):
    print("ERROR: not enough arguments. Format: insert_ip.py <input_folder_with_htmls> <output_folder_with_c_headers> <this_device_id>")
    quit()

# Create temporary folder
html_dir = sys.argv[1]
working_dir = os.getcwd() + "/generated_c_headers"
target_dir = sys.argv[2]
shutil.rmtree(working_dir, ignore_errors=True)
os.mkdir(working_dir)

default_ip_placeholder = "{IP_WILL_BE_PLACED_HERE}"
device_id_placeholder = "{DEVICE_ID_WILL_BE_PLACED_HERE}"
new_ip_range_begin = 100
device_id_str = sys.argv[3]
this_device_ap_ip = "192.168.4." + str(new_ip_range_begin + int(device_id_str))
for file_name in os.listdir(html_dir):
    if file_name.endswith(".html"):
        # Read HTML file
        input_file_path = os.path.join(html_dir, file_name)
        input_file = open(input_file_path, 'r')
        filedata = input_file.read()
        input_file.close()

        # Replace default IP with particular device IP
        filedata = filedata.replace(default_ip_placeholder, this_device_ap_ip)
        # Insert device ID
        filedata = filedata.replace(device_id_placeholder, device_id_str)

        # Generate content of header file
        new_header_name_wo_ext = os.path.splitext(file_name)[0]
        header_file_content = "#ifndef PAGE_" + new_header_name_wo_ext.upper() + "_H\n"
        header_file_content += "#define PAGE_" + new_header_name_wo_ext.upper() + "_H\n\n"
        header_file_content += "// This file was generated using xxd\n\n"
        header_file_content += "unsigned char page_index[] = {\n"
        # Compress HTML content
        compressed_filedata = gzip.compress(bytes(filedata, 'utf-8'))
        # ... and represent as C array
        header_file_content += ", ".join(bytes_to_c_array(compressed_filedata))
        header_file_content += "\n};\nunsigned int page_" + new_header_name_wo_ext + "_len = " + str(len(compressed_filedata)) + ";\n\n"
        header_file_content += "#endif\n"

        # Write result in header file
        header_output_filename = "page_" + new_header_name_wo_ext + ".h"
        output_header_file = open(os.path.join(working_dir, header_output_filename), 'w')
        output_header_file.write(header_file_content)
        output_header_file.close()
        

#  Clean up target directory
shutil.rmtree(target_dir, ignore_errors=True)
os.mkdir(target_dir)

# Copy all generated headers to target directory
for file_name in os.listdir(working_dir):
    if file_name.endswith(".h"):
        input_file_path = os.path.join(working_dir, file_name)
        output_file_path = os.path.join(target_dir, file_name)
        shutil.copyfile(input_file_path, output_file_path)

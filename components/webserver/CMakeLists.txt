idf_component_register(SRCS "webserver.cpp"
                    INCLUDE_DIRS "interface"
                    PRIV_REQUIRES hccapx_serializer pcap_serializer esp_http_server wifi_controller main)

# Create lists of input and output file names
file(GLOB_RECURSE HTML_FILES "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.html")
set(OUTPUT_FILES )
foreach(HTML_FILE ${HTML_FILES})
  get_filename_component(FILENAME ${HTML_FILE} NAME_WE)
  list(APPEND OUTPUT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/pages/page_${FILENAME}.h")
endforeach()

# Generate C header files containing pages content
add_custom_command(
  OUTPUT ${OUTPUT_FILES}
  COMMAND ${python} ${CMAKE_CURRENT_SOURCE_DIR}/utils/convert_html_to_header_file.py
                    ${CMAKE_CURRENT_SOURCE_DIR}/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/pages
                    ${CONFIG_DEVICE_ID}
  DEPENDS ${HTML_FILES}
  VERBATIM)

# Create new target for generated files. Add dependency of component on this new target
add_custom_target(generated_c_headers_target DEPENDS ${OUTPUT_FILES})
add_dependencies(${COMPONENT_LIB} generated_c_headers_target)

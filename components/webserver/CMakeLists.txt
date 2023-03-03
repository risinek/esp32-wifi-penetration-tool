idf_component_register(SRCS "webserver.cpp"
                    INCLUDE_DIRS "interface"
                    PRIV_REQUIRES hccapx_serializer pcap_serializer esp_http_server wifi_controller main)

# Run script to convert HTML pages to C header files
file(GLOB_RECURSE HTML_FILES ${CMAKE_CURRENT_SOURCE_DIR}/utils/*.html)
add_dependencies(${COMPONENT_LIB} generated_c_headers)
add_custom_target(generated_c_headers DEPENDS ${HTML_FILES})
add_custom_command(
  TARGET generated_c_headers
  COMMAND ${python} ${CMAKE_CURRENT_SOURCE_DIR}/utils/convert_html_to_header_file.py
                    ${CMAKE_CURRENT_SOURCE_DIR}/utils
                    ${CMAKE_CURRENT_SOURCE_DIR}/pages
                    ${CONFIG_DEVICE_ID}
  VERBATIM)

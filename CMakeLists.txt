# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-wifi-penetration-tool)

add_custom_command( 
     TARGET app 
     POST_BUILD 
     COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BIN} server_dir/${CONFIG_DEVICE_ID}/wpt${CONFIG_DEVICE_ID}.bin
)

idf_component_register(SRCS "bt_logs_forwarder.cpp" "serial_command_dispatcher.cpp" "attack_dos.cpp" "attack_method.cpp" "main.cpp" "attack_handshake.cpp" "attack_pmkid.cpp" "attack.cpp"
                    INCLUDE_DIRS .)

set_source_files_properties(main.cpp bt_logs_forwarder.cpp
    PROPERTIES COMPILE_FLAGS
    -std=c++14
)

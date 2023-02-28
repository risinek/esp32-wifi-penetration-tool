idf_component_register(SRCS "serial_command_dispatcher.cpp" "bluetooth.cpp" "attack_dos.c" "attack_method.c" "main.c" "attack_handshake.c" "attack_pmkid.c" "attack.c"
                    INCLUDE_DIRS .)

set_source_files_properties(bluetooth.cpp
    PROPERTIES COMPILE_FLAGS
    -std=c++14
)

idf_component_register(
    SRCS "wsl_bypasser.c"
    INCLUDE_DIRS "interface"
)
target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)
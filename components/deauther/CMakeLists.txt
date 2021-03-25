idf_component_register(
    SRCS "deauther.c"
    INCLUDE_DIRS "interface"
)
target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)
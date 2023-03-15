# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )

# By some reason it is impossible to add "tests" to COMPONENT_PRIV_REQUIRES variable in "if" statement
# in this CMake. As a workaround we are adding tests headers into COMPONENT_PRIV_INCLUDEDIRS
set(COMPONENT_PRIV_REQUIRES device_id nvs_flash bt)

if(DEFINED CONFIG_ENABLE_UNIT_TESTS)
  set(COMPONENT_PRIV_INCLUDEDIRS "${COMPONENT_PRIV_INCLUDEDIRS} ../tests")
endif()

file(GLOB COMPONENT_SRCS
  LIST_DIRECTORIES false
  "*.h"
  "*.cpp"
  "*.c"
  "*.S"
)
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

# target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++14)

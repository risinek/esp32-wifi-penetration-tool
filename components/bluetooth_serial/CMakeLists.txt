# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES
  "bt"
)
set(COMPONENT_PRIV_REQUIRES )

file(GLOB COMPONENT_SRCS
  LIST_DIRECTORIES false
  "*.h"
  "*.cpp"
  "*.c"
  "*.S"
)
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++14)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )

# Provide here list of components, tests for which are placed in this component
set(COMPONENT_PRIV_REQUIRES bluetooth_serial)

file(GLOB COMPONENT_SRCS
  LIST_DIRECTORIES false
  "*.h"
  "*.cpp"
  "*.c"
  "*.S"
)
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

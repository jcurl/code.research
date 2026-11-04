set(LIBRARY sddinfra)
set(SOURCES
    ../include/sdd/subscription.h
)
add_library(${LIBRARY} INTERFACE)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.22")
    # Fails on Ubuntu 20.04 (Focal), so enable for Ubuntu 22.04 and later.
    # CMake Error at lib/libstdext/src/CMakeLists.txt:6 (set_target_properties):
    #  INTERFACE_LIBRARY targets may only have whitelisted properties.  The
    #  property "OUTPUT_NAME" is not allowed.
    set_target_properties(${LIBRARY} PROPERTIES OUTPUT_NAME "sddinfra")
endif()

target_include_directories(${LIBRARY} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

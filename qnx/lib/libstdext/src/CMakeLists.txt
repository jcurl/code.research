set(LIBRARY libstdext)
set(SOURCES
    ../include/stdext/expected.h
)
add_library(${LIBRARY} INTERFACE)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.22")
    # Fails on Ubuntu 20.04 (Focal), so enable for Ubuntu 22.04 and later.
    # CMake Error at lib/libstdext/src/CMakeLists.txt:6 (set_target_properties):
    #  INTERFACE_LIBRARY targets may only have whitelisted properties.  The
    #  property "OUTPUT_NAME" is not allowed.
    set_target_properties(${LIBRARY} PROPERTIES OUTPUT_NAME "stdext")
endif()

target_include_directories(${LIBRARY} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# We don't want to format these files, to keep them similar to the original to
# make it easier to compare.
#
# target_clangformat(TARGET ${LIBRARY})

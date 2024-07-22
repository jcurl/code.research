cmake_minimum_required(VERSION 3.10)
project(rcutest CXX)

# See https://github.com/arsenm/sanitizers-cmake
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/modules/sanitizers" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/modules/clang-format" ${CMAKE_MODULE_PATH})
include(ClangFormat)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=bugprone-*,modernize-*,cppcoreguidelines-*,-cppcoreguidelines-avoid-magic-numbers")

set(SOURCES
    src/main.cpp
)

# So that we can find "config.h"
include_directories("${PROJECT_BINARY_DIR}")

add_executable(rcutest ${SOURCES})
if(CLANG_TIDY_EXE)
    set_target_properties(rcutest PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()

target_compile_features(rcutest PUBLIC cxx_std_17)

if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "DEBUG" CACHE STRING "" FORCE)
    endif()
endif()

if("${CMAKE_CXX_FLAGS}" STREQUAL "")
    if((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR
       (CMAKE_CXX_COMPILER_ID STREQUAL "GNU") OR
       (CMAKE_CXX_COMPILER_ID STREQUAL "QCC"))
        target_compile_options(rcutest PRIVATE -Wall -Wextra)

        if(CMAKE_BUILD_TYPE MATCHES DEBUG)
            message("-- Debug mode")
            target_compile_options(rcutest PRIVATE -O0)
        endif()
    endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    message("-- Sanitizers for Debugging")
    add_sanitizers(rcutest)
endif()

clangformat_setup(
    src/rcu.h
    src/main.cpp
)

# Finally write the configuration file dependent on what is found
# configure_file(${PROJECT_SOURCE_DIR}/src/config.h.in ${PROJECT_BINARY_DIR}/config.h)

cmake_minimum_required(VERSION 3.14)
project(rcutest C CXX)

include(GNUInstallDirs)

option(ENABLE_CLANG_TIDY "Enable checks using Clang-Tidy" ON)
option(ENABLE_TEST "Enable building tests" ON)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed")
endif()

# Default to DEBUG mode if not provided.
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" upper_CMAKE_BUILD_TYPE)
if (upper_CMAKE_BUILD_TYPE MATCHES "DEBUG")
    message("-- Debug mode")
    set(IS_DEBUG ON)
endif()

# See https://github.com/arsenm/sanitizers-cmake
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/modules/sanitizers" ${CMAKE_MODULE_PATH})
find_package(Sanitizers)

# See https://github.com/zemasoft/clangformat-cmake
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/modules/clang-format" ${CMAKE_MODULE_PATH})
include(ClangFormat)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/modules/qnx" ${CMAKE_MODULE_PATH})
include(qnx)

get_qnx_version()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        message(STATUS "LLVM clang-tidy is enabled with -DENABLE_CLANG_TIDY=on")
        # Must specify the C++17 version explicitly. When using GCC toolchain as
        # the compiler, it might decide that it doesn't need to provide the
        # "std=c++17" option (GCC 11.4 supports by default C++17). But then
        # running "clang-tidy" e.g. from clang toolchain 14.0 needs the option.
        # It isn't given as CMake doesn't do an additional check for clang-tidy.
        # If you compile using clang++, then the two come from the same
        # toolchain, so the assumption that the two are compatible holds.
        #
        # https://stackoverflow.com/questions/46808702/how-to-integrate-clang-tidy-with-cmake-lang-clang-tidy-and-msvc
        # https://gitlab.kitware.com/cmake/cmake/-/issues/24238
        if(CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang")
            set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
        else()
            set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--extra-arg-before=-std=c++17")
        endif()
    else()
        message(STATUS "LLVM clang-tidy is disabled (not found)")
    endif()
else()
    message(STATUS "LLVM clang-tidy is disabled with -DENABLE_CLANG_TIDY=off")
endif()

# So that we can find "config.h"
include_directories("${PROJECT_BINARY_DIR}")

if("${CMAKE_CXX_FLAGS}" STREQUAL "")
    if((CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang") OR
       (CMAKE_CXX_COMPILER_ID MATCHES "GNU") OR
       (CMAKE_CXX_COMPILER_ID MATCHES "QCC"))
        add_compile_options(-Wall -Wextra)

        if(IS_DEBUG)
            add_compile_options(-O0)
        endif()
    endif()
endif()

# Checking if we should enable coverage. Only do so in DEBUG and if testing is enabled
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/modules/coverage" ${CMAKE_MODULE_PATH})
include(code-coverage)
add_code_coverage()
if(CODE_COVERAGE)
    add_compile_options(-O0)
endif()

add_subdirectory(src)

if(ENABLE_TEST)
    # The `make install` target shouldn't install GTEST/GMOCK.
    set(INSTALL_GMOCK OFF)
    set(INSTALL_GTEST OFF)

    message(STATUS "Test suite enabled with -DENABLE_TEST=on (execute with make && ctest)")
    add_subdirectory(${CMAKE_SOURCE_DIR}/../external/googletest external/googletest)

    add_subdirectory(test)
    enable_testing()
else()
    message(STATUS "Test suite disabled with -DENABLE_TEST=off")
endif()

# Finally write the configuration file dependent on what is found
# configure_file(${PROJECT_SOURCE_DIR}/src/config.h.in ${PROJECT_BINARY_DIR}/config.h)

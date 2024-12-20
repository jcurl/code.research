cmake_minimum_required(VERSION 3.16)
project(benchmarks CXX)

include(GNUInstallDirs)

# Set the minimum C++ standard that should be used. When incrementing this,
# ensure that:
# - `VERBOSE=1 make` shows that the standard is really being passed on the
#   command line as you required, so that you have correct ABI. Google Benchmark
#   is known _not_ to do this.
# - Update the `target_compile_features` for `benchmark` and `benchmark_main` to
#   the C++ version required.
# - Update the `CMAKE_CXX_CLANG_TIDY` below to ensure that the standard is being
#   used, because CMake assumes that if GCC doesn't need the argument because it
#   defaults to the C++ standard being requested, then clang-tidy does too
#   (which is not true).
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (NOT QNXNTO AND NOT CYGWIN)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../cmake/modules/" ${CMAKE_MODULE_PATH})
include(research/qnx)
include(research/clang-tidy)

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

# So that we can find "config.h"
include_directories(${PROJECT_BINARY_DIR})

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

# This is for tests that are specific to the correct functioning and debugging
# of this repository, not necessarily expected to be used for comparison on
# targets.
option(ENABLE_TEST "Enable building repository tests" ON)

# The `make install` target shouldn't install GTEST/GMOCK.
set(INSTALL_GMOCK OFF)
set(INSTALL_GTEST OFF)
set(BUILD_GTEST ON)
set(BUILD_GMOCK ON)
add_subdirectory(${CMAKE_SOURCE_DIR}/../external/googletest external/googletest)

# Pull in Google Benchmark. Note, that the current comment in `external` has a
# `CMakeLists.txt` file that forces usage of C++14, where the tools in this
# project compile against C++17. This _may_ result in undefined behaviour of the
# ABI between the two C++ standards are not compatible. I've not found a way to
# force Google Benchmark to use the C++ version we want (but Google Test will).
# The variable `BENCHMARK_CXX_STANDARD` is not a cached variable, but this
# should ideally be set to the value of 17.
if (QNXNTO AND CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL 7.1)
    # QNX has -lregex needed for linking
    set(BENCHMARK_CXX_LIBRARIES regex)
endif()
set(BENCHMARK_ENABLE_INSTALL OFF)
set(BENCHMARK_ENABLE_DOXYGEN OFF)
set(BENCHMARK_INSTALL_DOCS OFF)
# Disable Google Benchmark Testing for: speed; QNX compilation issues; compiler
# errors (shown when using Clang) because Google Benchmark is only compatible
# with an older version of Google Test (and we can't override the CXX_STANDARD
# variable that Google Benchmark compiles with).
set(BENCHMARK_ENABLE_TESTING OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/../external/benchmark external/benchmark)
# Force the upgrade to C++17 when building the libraries we link to, ensuring
# ABI compatibility (our C++17 binaries link to a library also compiled against
# C++17, not C++14).
target_compile_features(benchmark PRIVATE cxx_std_17)
target_compile_features(benchmark_main PRIVATE cxx_std_17)

# Must enable CLANG-TIDY *after* google packages, so we don't interfere.
check_clang_tidy()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/libubench/include)
add_subdirectory(lib)

add_subdirectory(benchmarks)
add_subdirectory(tools)

if(ENABLE_TEST)
    message(STATUS "Test suite enabled with -DENABLE_TEST=on (execute with make && ctest)")
    enable_testing()
else()
    message(STATUS "Test suite disabled with -DENABLE_TEST=off")
endif()

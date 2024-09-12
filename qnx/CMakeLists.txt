cmake_minimum_required(VERSION 3.14)
project(benchmarks C CXX)

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
if (NOT QNXNTO)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

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

# The `make install` target shouldn't install GTEST/GMOCK.
set(INSTALL_GMOCK OFF)
set(INSTALL_GTEST OFF)
add_subdirectory(${CMAKE_SOURCE_DIR}/../external/googletest external/googletest)

# Pull in Google Benchmark. Note, that the current comment in `external` has a
# `CMakeLists.txt` file that forces usage of C++14, where the tools in this
# project compile against C++17. This _may_ result in undefined behaviour of the
# ABI between the two C++ standards are not compatible. I've not found a way to
# force Google Benchmark to use the C++ version we want (but Google Test will).
# The variable `BENCHMARK_CXX_STANDARD` is not a cached variable, but this
# should ideally be set to the value of 17.
if (QNXNTO)
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
option(ENABLE_CLANG_TIDY "Enable checks using Clang-Tidy" ON)
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

add_subdirectory(benchmarks)
add_subdirectory(tools)
#enable_testing()

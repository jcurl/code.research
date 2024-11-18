# Include this file as soon as possible after setting the top level project.
# This will run the function `get_qnx_version` when included.

# get_qnx_version()
#
# On output, sets the variable HAVE_QNX_VERSION to the version in the header
# files as the Version * 100 (e.g. 7.1.0 is 710, 8.0.0 is 800).
#
# On QNX 8.0 and later, the option `-lang-c++` produces a warning that this
# option is deprecated. The option `-xc++` should be used instead. The variable
# `CMAKE_CXX_COMPILE_OBJECT` and `CMAKE_CXX_LINK_EXECUTABLE` is patched to
# change the option for QNX 8.0.0 and later. Therefore, this function should run
# at the TOP LEVEL of your project and not in a function, to ensure those
# variables are in scope.
#
# Tested on QNX 7.1.0 and 8.0.0.

function(get_qnx_version)
    if(NOT DEFINED HAVE_QNX_VERSION)
        set(_qnx_ver_firstrun 1)
        _get_qnx_version_try_compile_c_cxx(_QNX_VERSION)
        set(HAVE_QNX_VERSION ${_QNX_VERSION} CACHE INTERNAL "Version of the QNX Compiler in use")
    endif()

    # If the version is wrong, or it wasn't found, then just exit. It would only be wrong
    # if the user overrode the value with something that aren't digits.
    string(REGEX MATCH "^([0-9]+)([0-9])([0-9])$" _qnx_ver_check "${HAVE_QNX_VERSION}")
    if("${_qnx_ver_check}" STREQUAL "")
        return()
    endif()

    set(_QNX_SYSTEM_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
    if("${CMAKE_SYSTEM_VERSION}" STREQUAL "")
        set(CMAKE_SYSTEM_VERSION "${_QNX_SYSTEM_VERSION}" CACHE STRING "CMake System Version")
	    set(CMAKE_SYSTEM_VERSION "${_QNX_SYSTEM_VERSION}" PARENT_SCOPE)
        if(${_qnx_ver_firstrun})
            message(STATUS "Checking for QNX - Consider setting CMAKE_SYSTEM_VERSION set to ${CMAKE_SYSTEM_VERSION}")
        endif()
    else()
        if(${_qnx_ver_firstrun})
            if(NOT "${_QNX_SYSTEM_VERSION}" STREQUAL "${CMAKE_SYSTEM_VERSION}")
                message(WARNING "Detected QNX version ${_QNX_SYSTEM_VERSION} doesn't match ${CMAKE_SYSTEM_VERSION}. Not overriding CMAKE_SYSTEM_VERSION.")
            endif()
        endif()
    endif()

    if(CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL 8.0)
        # In QNX 8.0 and later, the `-lang-c++` option is deprecated.
        # Unfortunately, it doesn't appear this can be set in the toolchain file
        # where it should really be.
        string(REPLACE "<CMAKE_CXX_COMPILER> -lang-c++" "<CMAKE_CXX_COMPILER> -xc++" CMAKE_CXX_COMPILE_OBJECT ${CMAKE_CXX_COMPILE_OBJECT})
        set(CMAKE_CXX_COMPILE_OBJECT ${CMAKE_CXX_COMPILE_OBJECT} PARENT_SCOPE)
        string(REPLACE "<CMAKE_CXX_COMPILER> -lang-c++" "<CMAKE_CXX_COMPILER>" CMAKE_CXX_LINK_EXECUTABLE ${CMAKE_CXX_LINK_EXECUTABLE})
        set(CMAKE_CXX_LINK_EXECUTABLE ${CMAKE_CXX_LINK_EXECUTABLE} PARENT_SCOPE)
    endif()
endfunction(get_qnx_version)

function(_get_qnx_version_try_compile_c_cxx QNX_VERSION)
    message(STATUS "Checking for QNX")

    # Make the test work on C and C++. Because we can't tell `try_compile` which
    # compiler to use for the file, we have to copy the source with a valid
    # extension the project is configured for.
    if(CMAKE_C_COMPILER_LOADED)
        if (NOT CMAKE_C_COMPILER_ID MATCHES "QCC")
            message(STATUS "Checking for QNX - QCC not in use (compiler id is ${CMAKE_C_COMPILER_ID})")
            set(${QNX_VERSION} "" PARENT_SCOPE)
            return()
        endif()
        set(_qnx_version_source_file "qnx-version.c")
    elseif(CMAKE_CXX_COMPILER_LOADED)
        if (NOT CMAKE_CXX_COMPILER_ID MATCHES "QCC")
            message(STATUS "Checking for QNX - QCC not in use (compiler id is ${CMAKE_CXX_COMPILER_ID})")
            set(${QNX_VERSION} "" PARENT_SCOPE)
            return()
        endif()
        set(_qnx_version_source_file "qnx-version.cpp")
    else()
        message(STATUS "Checking for QNX - No valid project language detected")
        set(${QNX_VERSION} "" PARENT_SCOPE)
        return()
    endif()

    set(_qnx_check_source "/* */\n")
    string(APPEND _qnx_version_source "
#include <sys/neutrino.h>
#include <sys/nto_version.h>

#ifndef __QNX__
# error Variable __QNX__ not found
#else
# define VALUE_TO_STRING(x) \"QNX_VERSION: \" #x
# define VALUE(x) VALUE_TO_STRING(x)
# if __QNX__ >= 800
#  pragma message(VALUE(__QNX__))
# else
#  pragma message(VALUE(_NTO_VERSION))
# endif
#endif

int main() { return 0; }
")

    file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/${_qnx_version_source_file}" "${_qnx_version_source}")
    try_compile(_qnx_compile_result
        ${CMAKE_BINARY_DIR}
        "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/${_qnx_version_source_file}"
        OUTPUT_VARIABLE _qnx_version_output)
    if (_qnx_compile_result)
        string(REGEX MATCH "QNX_VERSION: ([0-9]+)" _qnx_version "${_qnx_version_output}")
        set(_qnx_version ${CMAKE_MATCH_1})
        if("${_qnx_version}" STREQUAL "")
            set(${QNX_VERSION} "" PARENT_SCOPE)
            message(STATUS "Checking for QNX - Not found")
        else()
            set(${QNX_VERSION} ${_qnx_version} PARENT_SCOPE)
            message(STATUS "Checking for QNX - Found version ${_qnx_version}")
        endif()
    else()
        message(STATUS "Checking for QNX - Error running test")
        set(${QNX_VERSION} "" PARENT_SCOPE)
    endif()
endfunction(_get_qnx_version_try_compile_c_cxx)

get_qnx_version()

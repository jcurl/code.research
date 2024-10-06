set(QNX_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})

# get_qnx_version()
#
# On output, sets the variable HAVE_QNX_VERSION to the version in the header
# files as the Version * 100 (e.g. 7.1.0 is 710, 8.0.0 is 800).
#
# Tested on QNX 7.1.0 and 8.0.0.
function(get_qnx_version_try_compile_c_cxx result)
    message(STATUS "Checking for QNX")

    # Make the test work on C and C++. Because we can't tell `try_compile` which
    # compiler to use for the file, we have to copy the source with a valid
    # extension the project is configured for.
    get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
    if("C" IN_LIST languages)
        if (NOT CMAKE_C_COMPILER_ID MATCHES "QCC")
            message(STATUS "Checking for QNX - QCC not in use (compiler id is ${CMAKE_C_COMPILER_ID})")
            return()
        endif()
        set(qnx_version_source_file "${QNX_CMAKE_DIR}/src/qnx-version.c")
    elseif("CXX" IN_LIST languages)
        if (NOT CMAKE_CXX_COMPILER_ID MATCHES "QCC")
            message(STATUS "Checking for QNX - QCC not in use (compiler id is ${CMAKE_CXX_COMPILER_ID})")
            return()
        endif()
        set(qnx_version_source_file "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/qnx-version.cpp")
        file(READ "${QNX_CMAKE_DIR}/src/qnx-version.c" qnx_version_source)
        file(WRITE ${qnx_version_source_file} "${qnx_version_source}\n")
    else()
        message(STATUS "Checking for QNX - No valid project language detected")
        return()
    endif()

    try_compile(compile_result "${CMAKE_BINARY_DIR}"
        SOURCES ${qnx_version_source_file}
        OUTPUT_VARIABLE qnx_version_output)
    if (compile_result)
        string(REGEX MATCH "QNX_VERSION: ([0-9]+)" qnx_version "${qnx_version_output}")
        set(qnx_version ${CMAKE_MATCH_1})
        if("${qnx_version}" STREQUAL "")
            set(${result} "" PARENT_SCOPE)
            message(STATUS "Checking for QNX - Not found")
        else()
            set(${result} ${qnx_version} PARENT_SCOPE)
            message(STATUS "Checking for QNX - Found version ${qnx_version}")
        endif()
    else()
        message(STATUS "Checking for QNX - Error running test")
        set(${result} "" PARENT_SCOPE)
    endif()
endfunction(get_qnx_version_try_compile_c_cxx)

function(get_qnx_version)
    if("${HAVE_QNX_VERSION}" STREQUAL "")
        get_qnx_version_try_compile_c_cxx(qnx_version)
        if(NOT "${qnx_version}" STREQUAL "")
            set(HAVE_QNX_VERSION ${qnx_version} CACHE STRING "Version of the QNX Compiler in use")
        endif()
    else()
        message(STATUS "Checking for QNX - Found version ${HAVE_QNX_VERSION}")
    endif()
endfunction(get_qnx_version)

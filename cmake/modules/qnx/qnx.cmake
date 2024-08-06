set(QNX_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})

# get_qnx_version()
#
# On output, sets the variable QNX_VERSION to either `NONE` (not found), or the
# version in the header files as the Version * 100 (e.g. 7.1.0 is 710).
function(get_qnx_version)
    if(NOT "${QNX_VERSION}" STREQUAL "")
        return()
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "QCC")
        message(STATUS "Checking for QNX - QCC not in use")
        set(QNX_VERSION NONE PARENT_SCOPE)
    else()
        try_compile(result "${CMAKE_BINARY_DIR}"
            SOURCES "${QNX_CMAKE_DIR}/src/qnx-version.c"
            OUTPUT_VARIABLE qnx_version_output)
        string(REGEX MATCH "QNX_VERSION: ([0-9]+)" qnx_version "${qnx_version_output}")
        set(qnx_version ${CMAKE_MATCH_1})
        if("${qnx_version}" STREQUAL "")
            set(QNX_VERSION NONE PARENT_SCOPE)
        else()
            set(QNX_VERSION ${qnx_version} PARENT_SCOPE)
        endif()

        message(STATUS "Checking for QNX - Found version ${qnx_version}")
    endif()
endfunction(get_qnx_version)

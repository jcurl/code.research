# find_libsocket([LIBNAME variable] [PROJECT name] [FATAL])
#
# Checks if the "bind" function is available, and if the library socket must be
# added to the project.
#
# If "FATAL" is given, then the function aborts configuration if the "bind"
# function cannot be found with a FATAL_ERROR.
#
# If "PROJECT ${BINARY}", then automatically add the library to the project
# ${BINARY} with 'target_link_libraries'.
#
# If "LIBNAME variable", then put the library that should be added to the
# project in "variable". You can add the library manually if you want to. If a
# library must be linked, "variable" is the name of the library. Add
# `target_link_libraries(${BINARY} variable)` after the call. If socket
# functionality isn't available, "variable" is "none_found".
#
# The variable "CMAKE_REQUIRED_LIBRARIES" is updated automatically, so calls to
# "check_symbol_exists" will work (e.g. searching for "sendmmsg").
#
# Example:
#
# ```cmake
# find_libsocket(PROJECT ${BINARY} FATAL)
# ```
#
# Checks that "bind" is available, and automatically adds "libsocket" to the
# project target link libraries if needed. If the "bind" symbol cannot be found,
# then the a FATAL_ERROR is reported.
function(find_libsocket)
    set(_options FATAL)
    set(_singleargs PROJECT LIBNAME)
    cmake_parse_arguments(PARSE_ARGV 0 arg "${_options}" "${_singleargs}" "")
    if(NOT arg_LIBNAME AND NOT arg_PROJECT)
        message(FATAL_ERROR "function 'check_libsocket' should have argument PROJECT or LIBNAME")
    endif()

    set(CMAKE_REQUIRED_QUIET 1)
    message(STATUS "Looking for libsocket")

    check_symbol_exists(bind "arpa/inet.h" HAVE_LIBSOCKET)
    if(HAVE_LIBSOCKET)
        message(STATUS "Looking for libsocket - available, not needed")
        if(arg_LIBNAME)
            set(${arg_LIBNAME} "" PARENT_SCOPE)
        endif()
        return()
    endif()

    if (NOT "socket" IN_LIST CMAKE_REQUIRED_LIBRARIES)
        list(APPEND CMAKE_REQUIRED_LIBRARIES socket)
    endif()

    check_library_exists("socket" "bind" "" HAVE_LIBSOCKET_LIB)
    if(HAVE_LIBSOCKET_LIB)
        message(STATUS "Looking for libsocket - found -lsocket")
        if(arg_LIBNAME)
            set(${arg_LIBNAME} "socket" PARENT_SCOPE)
        endif()
        if(arg_PROJECT)
            target_link_libraries(${arg_PROJECT} "socket")
        endif()

        # This is so 'check_symbol_exists' will find libsocket functions
        set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} PARENT_SCOPE)
        return()
    endif()

    if(arg_LIBNAME)
        set(${arg_LIBNAME} "none_found" PARENT_SCOPE)
    endif()
    if(NOT arg_FATAL)
        message(STATUS "Looking for libsocket - not found")
    else()
        message(FATAL_ERROR "Looking for libsocket - not found")
    endif()
endfunction(find_libsocket)

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
# then the a FATAL_ERROR is reported if so configured.
#
# On output of this function, the variable HAVE_LIBSOCKET is set to 1 if it is
# available. The variable HAVE_LIBSOCKET_LIB contains the name of the library to
# link.
function(find_libsocket)
    set(_options FATAL)
    set(_singleargs PROJECT LIBNAME)
    cmake_parse_arguments(PARSE_ARGV 0 arg "${_options}" "${_singleargs}" "")
    if(NOT arg_LIBNAME AND NOT arg_PROJECT)
        message(FATAL_ERROR "function 'check_libsocket' should have argument PROJECT or LIBNAME")
    endif()

    set(CMAKE_REQUIRED_QUIET 1)
    if(NOT DEFINED HAVE_LIBSOCKET)
        message(STATUS "Looking for libsocket")

        check_symbol_exists(bind "sys/socket.h" HAVE_LIBSOCKET)
        if(HAVE_LIBSOCKET)
            message(STATUS "Looking for libsocket - found, linking not needed")
            set(HAVE_LIBSOCKET 1 CACHE STRING "Have symbol bind" FORCE)
            if(arg_LIBNAME)
                set(${arg_LIBNAME} "" PARENT_SCOPE)
            endif()
            return()
        endif()

        # For now, we only know of libsocket. If there happens to be other
        # libraries providing socket functionality, we put this in a loop.
        if (NOT "socket" IN_LIST CMAKE_REQUIRED_LIBRARIES)
            list(APPEND CMAKE_REQUIRED_LIBRARIES "socket")
        endif()

        check_library_exists("socket" "bind" "" HAVE_LIBSOCKET_LIB)
        if(HAVE_LIBSOCKET_LIB)
            set(HAVE_LIBSOCKET 1 CACHE STRING "Have symbol bind" FORCE)
            set(HAVE_LIBSOCKET_LIB "socket" CACHE STRING "Name of the library to link for sockets" FORCE)
            message(STATUS "Looking for libsocket - found -lsocket")
            if(arg_LIBNAME)
                set(${arg_LIBNAME} "socket" PARENT_SCOPE)
            endif()
            if(arg_PROJECT)
                target_link_libraries(${arg_PROJECT} "socket")
            endif()

            # This is so 'check_symbol_exists' will find libsocket functions
            set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} PARENT_SCOPE)
        else()
            if(arg_LIBNAME)
                set(${arg_LIBNAME} "" PARENT_SCOPE)
            endif()
            if(NOT arg_FATAL)
                message(STATUS "Looking for libsocket - not found")
            else()
                message(FATAL_ERROR "Looking for libsocket - not found")
            endif()
        endif()
    else()
        # Either, or both of HAVE_LIBSOCKET or HAVE_LIBSOCKET_LIB can be here.
        # We dont' do the checks a second time, but must set all variables as if
        # we had.

        # message(STATUS "Looking for libsocket (cached)")
        if(HAVE_LIBSOCKET)
            # if(DEFINED HAVE_LIBSOCKET_LIB AND HAVE_LIBSOCKET_LIB)
            #     message(STATUS "Looking for libsocket (cached) - found -l${HAVE_LIBSOCKET_LIB}")
            # else()
            #     message(STATUS "Looking for libsocket (cached) - found, linking not needed")
            # endif()
            if(arg_LIBNAME)
                set(${arg_LIBNAME} "${HAVE_LIBSOCKET_LIB}" PARENT_SCOPE)
            endif()
            if(DEFINED HAVE_LIBSOCKET_LIB AND HAVE_LIBSOCKET_LIB)
                if (NOT "${HAVE_LIBSOCKET_LIB}" IN_LIST CMAKE_REQUIRED_LIBRARIES)
                    list(APPEND CMAKE_REQUIRED_LIBRARIES "${HAVE_LIBSOCKET_LIB}")
                    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} PARENT_SCOPE)
                endif()
                if(arg_PROJECT)
                    target_link_libraries(${arg_PROJECT} "${HAVE_LIBSOCKET_LIB}")
                endif()
            endif()
        else()
            if(NOT arg_FATAL)
                # message(STATUS "Looking for libsocket (cached) - not found")
            else()
                message(FATAL_ERROR "Looking for libsocket (cached) - not found")
            endif()
        endif()
    endif()
endfunction(find_libsocket)

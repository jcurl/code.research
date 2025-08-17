# check_symbol_lib_exists(symbol files variable LIB lib TARGET x FATAL)
#
# Checks if <symbol> exists with the included <files>. If it is, then <variable>
# is set as cached.
#
# If <lib> is not defined, and <symbol> is not found, then <variable> is set to
# be empty like check_symbol_exists. If FATAL is present, the function aborts
# with a fatal message.
#
# If <lib> is defined, and <symbol> is not found, then the test is done a second
# time
#
# If the symbol is found in the library requested, the library is added to the
# parent scope of CMAKE_REQUIRED_LIBRARIES. This allows looking for symbols in
# the list line, assuming the library will be linked.
function(check_symbol_lib_exists SYMBOL FILES VARIABLE)
    if(NOT DEFINED SYMBOL OR "x${SYMBOL}" STREQUAL "x")
        message(FATAL_ERROR "check_symbol_lib_exists: SYMBOL not defined")
    endif()
    if(NOT "${SYMBOL}" MATCHES "^[a-zA-Z_][a-zA-Z0-9_]*$")
        message(FATAL_ERROR "check_symbol_lib_exists: SYMBOL is an invalid symbol")
    endif()
    if(NOT DEFINED FILES OR "x${FILES}" STREQUAL "x")
        message(FATAL_ERROR "check_symbol_lib_exists: FILES not defined")
    endif()

    set(_options FATAL)
    set(_singleargs TARGET LIB)
    cmake_parse_arguments(PARSE_ARGV 3 arg "${_options}" "${_singleargs}" "")

    # ${VARIABLE}
    #   = 1 if found
    #   "" if not found
    #   not defined if not searched for
    # ${VARIABLE}_LINKLIBRARY
    #   "libname", library to link to
    #   not defined, no library needed if ${VARIABLE} = 1

    # Don't output the test cases. Because this is a function, the value is only
    # local to here and it doesn't need to be set back.
    set(user_CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET})
    set(CMAKE_REQUIRED_QUIET 1)

    if(${VARIABLE})
        # If ${VARIABLE} is already set (it was previously found) we don't need
        # to look again. But if the TARGET is specified, we still need to add it
        # if it isn't already.
        if(NOT ${VARIABLE}_LINKLIBRARY)
            # variable_LINK_LIBRARY is not defined or is empty. Then it's found
            # and we don't need to link anything.
            if (NOT DEFINED arg_LIB)
                if(NOT user_CMAKE_REQUIRED_QUIET)
                    message(STATUS "Looking for ${SYMBOL}")
                    message(STATUS "Looking for ${SYMBOL} - found (cached), no linking needed")
                endif()
            # else()
                # Suppress spamming the output. The user called this function
                # prior and it was found. They're probably calling this again
                # with another library and will check what was found after all
                # the tests.
                #
                # Has also the effect, that if the user looks in two different
                # modules for the same function, the second check will be
                # automatically silent because it's cached.
                #
                # We don't know where we're called from, so we can't know if a
                # different function is now calling us to give a useful
                # diagnostic.
            endif()
            return()
        endif()

        if(NOT DEFINED arg_LIB)
            if(NOT user_CMAKE_REQUIRED_QUIET)
                message(STATUS "Looking for ${SYMBOL}")
            endif()
            # Somehow it was found in a library, but user didn't specify a
            # library to link. We should have the same behaviour when cached as
            # when doing the actual search. This indicates that the cmake
            # scripts changed since the last invocation.
            if(NOT ${arg_FATAL})
                if(NOT user_CMAKE_REQUIRED_QUIET)
                    message(STATUS "Looking for ${SYMBOL} - not found (cached)")
                endif()
            else()
                message(FATAL_ERROR "Looking for ${SYMBOL} - not found (cached)")
            endif()
            return()
        endif()

        if(NOT ${arg_LIB} IN_LIST CMAKE_REQUIRED_LIBRARIES)
            list(APPEND CMAKE_REQUIRED_LIBRARIES ${arg_LIB})
        endif()

        if("${${VARIABLE}_LINKLIBRARY}" STREQUAL "${arg_LIB}")
            if(arg_TARGET)
                # Don't add to the target if it is already added
                _check_symbol_lib_exists_in_target(_FOUND ${arg_TARGET} ${${VARIABLE}_LINKLIBRARY})
                if(${_FOUND})
                    if(NOT user_CMAKE_REQUIRED_QUIET)
                        message(STATUS "Looking for ${SYMBOL}")
                        message(STATUS "Looking for ${SYMBOL} - found (cached) in lib${${VARIABLE}_LINKLIBRARY}, already added to ${arg_TARGET}")
                    endif()
                else()
                    if(NOT user_CMAKE_REQUIRED_QUIET)
                        message(STATUS "Looking for ${SYMBOL}")
                    endif()
                    target_link_libraries(${arg_TARGET} PRIVATE ${${VARIABLE}_LINKLIBRARY})
                    if(NOT user_CMAKE_REQUIRED_QUIET)
                        message(STATUS "Looking for ${SYMBOL} - found (cached) in lib${${VARIABLE}_LINKLIBRARY}, added to ${arg_TARGET}")
                    endif()
                endif()
            else()
                if(NOT user_CMAKE_REQUIRED_QUIET)
                    message(STATUS "Looking for ${SYMBOL}")
                    message(STATUS "Looking for ${SYMBOL} - found (cached) in lib${${VARIABLE}_LINKLIBRARY}")
                endif()
            endif()
            set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} PARENT_SCOPE)
            return()
        endif()

        if(NOT ${arg_FATAL})
            if(NOT user_CMAKE_REQUIRED_QUIET)
                message(STATUS "Looking for ${SYMBOL}")
                message(STATUS "Looking for ${SYMBOL} - not found (cached) in ${arg_LIB}")
            endif()
        else()
            if(NOT user_CMAKE_REQUIRED_QUIET)
                message(STATUS "Looking for ${SYMBOL}")
            endif()
            message(FATAL_ERROR "Looking for ${SYMBOL} - not found (cached) in ${arg_LIB}")
        endif()
        return()
    endif()

    if(NOT user_CMAKE_REQUIRED_QUIET)
        message(STATUS "Looking for ${SYMBOL}")
    endif()

    # At this point, `${VARIABLE}` is undefined, so it will always do the check.
    check_symbol_exists("${SYMBOL}" "${FILES}" "${VARIABLE}")
    if(${VARIABLE})
        if(NOT user_CMAKE_REQUIRED_QUIET)
            message(STATUS "Looking for ${SYMBOL} - found, no linking needed")
        endif()

        # Regardless of the variable, we clear it.
        set(${VARIABLE}_LINKLIBRARY "" CACHE INTERNAL "Library to link for symbol ${SYMBOL}")
        return()
    endif()

    if(NOT DEFINED arg_LIB)
        if(NOT ${arg_FATAL})
            if(NOT user_CMAKE_REQUIRED_QUIET)
                message(STATUS "Looking for ${SYMBOL} - not found")
            endif()
        else()
            message(FATAL_ERROR "Looking for ${SYMBOL} - not found")
        endif()
        return()
    endif()

    # Didn't find it until now. Search in a library.
    if(NOT ${arg_LIB} IN_LIST CMAKE_REQUIRED_LIBRARIES)
        list(APPEND CMAKE_REQUIRED_LIBRARIES ${arg_LIB})
    endif()

    # Need to unset the cached variable, so it will now search with the updated
    # CMAKE_REQUIRED_LIBRARIES. Else it will "skip" it even if it can now be
    # found with the new library.
    unset("${VARIABLE}" CACHE)
    check_symbol_exists("${SYMBOL}" "${FILES}" "${VARIABLE}")
    if(${VARIABLE})
        set(${VARIABLE}_LINKLIBRARY "${arg_LIB}" CACHE INTERNAL "Library to link for symbol ${SYMBOL}")
        if(arg_TARGET)
            # Don't add to the target if it is already added
            _check_symbol_lib_exists_in_target(_FOUND ${arg_TARGET} ${${VARIABLE}_LINKLIBRARY})
            if(${_FOUND})
                if(NOT user_CMAKE_REQUIRED_QUIET)
                    message(STATUS "Looking for ${SYMBOL} - found in lib${${VARIABLE}_LINKLIBRARY}, already added to ${arg_TARGET}")
                endif()
            else()
                target_link_libraries(${arg_TARGET} ${${VARIABLE}_LINKLIBRARY})
                if(NOT user_CMAKE_REQUIRED_QUIET)
                    message(STATUS "Looking for ${SYMBOL} - found in lib${${VARIABLE}_LINKLIBRARY}, added to ${arg_TARGET}")
                endif()
            endif()
        else()
            if(NOT user_CMAKE_REQUIRED_QUIET)
                message(STATUS "Looking for ${SYMBOL} - found in lib${${VARIABLE}_LINKLIBRARY}")
            endif()
        endif()
        set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} PARENT_SCOPE)
        return()
    endif()

    if(NOT ${arg_FATAL})
        if(NOT user_CMAKE_REQUIRED_QUIET)
            message(STATUS "Looking for ${SYMBOL} - not found in ${arg_LIB}")
        endif()
    else()
        message(FATAL_ERROR "Looking for ${SYMBOL} - not found in ${arg_LIB}")
    endif()
endfunction()

macro(_check_symbol_lib_exists_in_target VARIABLE TARGET LIB)
    unset(${VARIABLE})
    get_target_property(_LIBS ${TARGET} LINK_LIBRARIES)
    foreach(_TESTLIB ${_LIBS})
        if("${_TESTLIB}" STREQUAL "${LIB}")
            set(${VARIABLE} 1)
        endif()
    endforeach()
endmacro()

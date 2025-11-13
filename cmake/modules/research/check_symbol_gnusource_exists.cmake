# check_symbol_gnusource__exists(<symbol> <files> <variable> [FORCE] [MSG msg])
#
# Usage is similar to `check_symbol_exists()`, but this checks for the existence
# of the symbol and then checks again with GNU_SOURCE set.
#
# Check that the `<symbol>` is available after including headers `<files>` and
# store the result in `<variable>`. Specify the list of files in one argument as
# a semicolon-separated list. `<variable>` will be created as an internal cache
# variable.
#
# Usually, once the variable is determined as not found, calling the function
# again will result in it skipping the test. However, some tests may want to try
# a second time with a different header, so provide the argument `FORCE` to
# require the check.
#
# Because a user might call this macro multiple times looking for a symbol, they
# can provide an optional message with MSG "in if/net.h" or what they'd like to
# tell the user.
#
# The following variables may be set before calling this macro to modify the way
# the check is run:
#
# `CMAKE_REQUIRED_FLAGS`: string of compile command line flags.
#
# `CMAKE_REQUIRED_INCLUDES`: a ;-list of header search paths to pass to the
# compiler.
#
# `CMAKE_REQUIRED_QUIET`: execute quietly without messages.
#
# This implementation is based on `macro(CHECK_SYMBOL_EXISTS)` from CMake
# v3.16.9. It was created because if the symbol is defined in an enumeration,
# then `check_symbol_exists()` fails. So this is called first, followed by a
# second specific test if it might be an enum.
function(check_symbol_gnusource_exists SYMBOL FILES VARIABLE)
    if(NOT DEFINED SYMBOL OR "x${SYMBOL}" STREQUAL "x")
        message(FATAL_ERROR "check_symbol_gnusource_exists: SYMBOL not defined")
    endif()
    if(NOT "${SYMBOL}" MATCHES "^[a-zA-Z_][a-zA-Z0-9_]*$")
        message(FATAL_ERROR "check_symbol_gnusource_exists: SYMBOL is an invalid symbol")
    endif()
    if(NOT DEFINED FILES OR "x${FILES}" STREQUAL "x")
        message(FATAL_ERROR "check_symbol_source_exists: FILES not defined")
    endif()

    set(_options FORCE)
    set(_singleargs MSG)
    cmake_parse_arguments(PARSE_ARGV 3 arg "${_options}" "${_singleargs}" "")
    if(arg_MSG)
        set(_MSG " ${arg_MSG}")
    endif()

    if(arg_FORCE OR NOT DEFINED "${VARIABLE}" OR "x${${VARIABLE}}" STREQUAL "x${VARIABLE}")
        set(user_CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET})
        set(CMAKE_REQUIRED_QUIET 1)

        if(NOT user_CMAKE_REQUIRED_QUIET)
            message(STATUS "Looking for ${SYMBOL}")
        endif()

        # If we don't unset this, `check_symbol_exists` may return TRUE based on
        # a cached variable, but with the wrong header files being checked.
        unset(${VARIABLE} CACHE)
        check_symbol_exists("${SYMBOL}" "${FILES}" "${VARIABLE}")
        if(NOT ${VARIABLE})
            unset(${VARIABLE} CACHE)

            cmake_push_check_state()
            list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
            check_symbol_exists("${SYMBOL}" "${FILES}" "${VARIABLE}")
            cmake_pop_check_state()
            if (${VARIABLE})
                list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
            endif()

            if(NOT user_CMAKE_REQUIRED_QUIET)
                if(${VARIABLE})
                    add_compile_definitions(_GNU_SOURCE)
                    message(STATUS "Looking for ${SYMBOL} - found (_GNU_SOURCE)${_MSG}")
                else()
                    message(STATUS "Looking for ${SYMBOL} - not found${_MSG}")
                endif()
            endif()
            set(${VARIABLE} "${${VARIABLE}}" CACHE INTERNAL "Symbol ${SYMBOL}")
        else()
            if(NOT user_CMAKE_REQUIRED_QUIET)
                message(STATUS "Looking for ${SYMBOL} - found${_MSG}")
            endif()
        endif()
    endif()
endfunction()

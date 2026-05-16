include(CheckTypeSize)

# check_sizeof_off_t()
#
# Checks the size of `off_t`. If necessary, set macros and report what is
# required for 64-bit.
#
# - CMAKE_REQUIRED_QUIET: If set to TRUE, the messages are not printed.
#
# On output
# - HAVE_SIZEOF_OFF_T is TRUE if available
# - SIZEOF_OFF_T the size, in bytes (4 for 32-bit, 8 for 64-bit. This value should
#   only be checked if HAVE_SIZEOF_OFF_T is TRUE.
# - HAVE_FILE_OFFSET_BITS_64 is required for 64-bit, 32-bit otherwise. This value
#   should only be checked if SIZEOF_OFF_T is 8.
function(check_sizeof_off_t)
    if(NOT DEFINED HAVE_SIZEOF_OFF_T)
        set(user_CMAKE_REQUIRED_QUIET "${CMAKE_REQUIRED_QUIET}")
        set(CMAKE_REQUIRED_QUIET 1)
        set(CMAKE_REQUIRED_INCLUDES "unistd.h")

        if(NOT user_CMAKE_REQUIRED_QUIET)
            message(STATUS "Looking for off_t")
        endif()

        check_type_size("off_t" SIZEOF_OFF_T LANGUAGE C)

        if (!${HAVE_SIZEOF_OFF_T})
            message(STATUS "Looking for off_t - not found")
            return()
        endif()

        set(HAVE_FILE_OFFSET_BITS_64 FALSE)
        if (${SIZEOF_OFF_T} EQUAL 4)
            set(CMAKE_REQUIRED_DEFINITIONS "-D_FILE_OFFSET_BITS=64")
            unset(HAVE_SIZEOF_OFF_T CACHE)
            unset(SIZEOF_OFF_T CACHE)

            check_type_size("off_t" SIZEOF_OFF_T LANGUAGE C)

            if (!${HAVE_SIZEOF_OFF_T})
                message(FATAL_ERROR "Looking for off_t - type 'off_t' not found in 'unistd.h' when _FILE_OFFSET_BITS=64")
                return()
            endif()

            if (${SIZEOF_OFF_T} EQUAL 8)
                set(HAVE_FILE_OFFSET_BITS_64 TRUE)
            endif()

            unset(CMAKE_REQUIRED_DEFINITIONS)
        endif()

        unset(CMAKE_REQUIRED_INCLUDES)

        set(HAVE_FILE_OFFSET_BITS_64 ${HAVE_FILE_OFFSET_BITS_64} CACHE STRING "if off_t 64-bit requires -D_FILE_OFFSET_BITS=64" FORCE)

        if (HAVE_FILE_OFFSET_BITS_64)
            if(NOT user_CMAKE_REQUIRED_QUIET)
                message(STATUS "Looking for off_t - found ${SIZEOF_OFF_T} bytes with -D_FILE_OFFSET_BITS=64")
            endif()
        else()
            if(NOT user_CMAKE_REQUIRED_QUIET)
                message(STATUS "Looking for off_t - found ${SIZEOF_OFF_T} bytes")
            endif()
        endif()
    endif()
endfunction()

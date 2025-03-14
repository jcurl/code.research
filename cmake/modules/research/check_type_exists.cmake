# check_type_exists(<type> <files> <variable>)
#
# Usage is similar to `check_symbol_exists()`, but this checks for the existence
# of the type.
#
# Check that the `<type>` is available after including headers `<files>` and
# store the result in `<variable>`. Specify the list of files in one argument as
# a semicolon-separated list. `<variable>` will be created as an internal cache
# variable.
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
# v3.16.9.
function(check_type_exists SYMBOL FILES VARIABLE)
    if(NOT DEFINED SYMBOL OR "x${SYMBOL}" STREQUAL "x")
        message(FATAL_ERROR "check_type_exists: SYMBOL not defined")
    endif()

    set(user_CMAKE_REQUIRED_QUIET ${CMAKE_REQUIRED_QUIET})
    set(CMAKE_REQUIRED_QUIET 1)

    if(CMAKE_CXX_COMPILER_LOADED)
        # Search for C++ first, in case the use is looking for a C++ symbol. We
        # assume that C++ is a superset of C.
        if(NOT "${SYMBOL}" MATCHES "^[a-zA-Z_][:<>a-zA-Z0-9_ ]*$")
            message(FATAL_ERROR "check_type_exists: SYMBOL is an invalid symbol")
        endif()
        __check_type_exists_impl("CheckTypeExists.cxx" "${SYMBOL}" "${FILES}" "${VARIABLE}")
    elseif(CMAKE_C_COMPILER_LOADED)
        if(NOT "${SYMBOL}" MATCHES "^[a-zA-Z_][a-zA-Z0-9_ ]*$")
            message(FATAL_ERROR "check_type_exists: SYMBOL is an invalid symbol")
        endif()
        __check_type_exists_impl("CheckTypeExists.c" "${SYMBOL}" "${FILES}" "${VARIABLE}")
    else()
        message(FATAL_ERROR "CHECK_TYPE_EXISTS needs either C or CXX language enabled")
    endif()
endfunction()

macro(__check_type_exists_impl SOURCEFILE SYMBOL FILES VARIABLE)
    get_filename_component(_SN "${SOURCEFILE}" NAME_WLE)
    get_filename_component(_SE "${SOURCEFILE}" LAST_EXT)
    set(_SOURCEFILE_ISTYPE "${_SN}_T${_SE}")
    set(_SOURCEFILE_ISVAR "${_SN}_V${_SE}")
    unset(_SN)
    unset(_SE)

    if(NOT DEFINED "${VARIABLE}" OR "x${${VARIABLE}}" STREQUAL "x${VARIABLE}")
        set(MACRO_CHECK_TYPE_EXISTS_FLAGS ${CMAKE_REQUIRED_FLAGS})
        if(CMAKE_REQUIRED_INCLUDES)
            set(CMAKE_TYPE_EXISTS_INCLUDES "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_REQUIRED_INCLUDES}")
        else()
            set(CMAKE_TYPE_EXISTS_INCLUDES)
        endif()

        # Notes from Autoconf "types.m4" on how to check if a type exists.
        #
        # See
        #  http://git.savannah.gnu.org/gitweb/?p=autoconf.git;a=blob;f=lib/autoconf/types.m4;h=508b64a21e159725e0233b7289053eb77ec1dc43;hb=HEAD
        #
        # We check if `sizeof (TYPE);` compiles. If it does, it can be a type or
        # a variable. If not, then fail.
        #
        # Then we check if `sizeof ((TYPE));` fails. If it does, it's a type.
        # Else it's a variable.

        set(_CHECK_TYPE_SOURCE_ISTYPE "/* */\n")
        foreach(FILE ${FILES})
            string(APPEND _CHECK_TYPE_SOURCE_ISTYPE "#include <${FILE}>\n")
        endforeach()
        string(APPEND _CHECK_TYPE_SOURCE_ISTYPE "
int main(int argc, char** argv)
{
  (void)argv;
  (void)argc;
  if (sizeof(${SYMBOL})) return 1;
  return 0;
}")

        set(_CHECK_TYPE_SOURCE_ISVAR "/* */\n")
        foreach(FILE ${FILES})
            string(APPEND _CHECK_TYPE_SOURCE_ISVAR "#include <${FILE}>\n")
        endforeach()
        string(APPEND _CHECK_TYPE_SOURCE_ISVAR "
int main(int argc, char** argv)
{
  (void)argv;
  (void)argc;
  if (sizeof((${SYMBOL}))) return 1;
  return 0;
}")

        if(NOT user_CMAKE_REQUIRED_QUIET)
            message(STATUS "Looking for type ${SYMBOL}")
        endif()

        file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/${_SOURCEFILE_ISTYPE}" "${_CHECK_TYPE_SOURCE_ISTYPE}")
        try_compile(${VARIABLE}
            ${CMAKE_BINARY_DIR}
            "${_SOURCEFILE_ISTYPE}"
            COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
            CMAKE_FLAGS
            -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_TYPE_EXISTS_FLAGS}
            "${CMAKE_TYPE_EXISTS_INCLUDES}"
            OUTPUT_VARIABLE OUTPUT)
        if(${VARIABLE})
            file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
                "Determining if the ${SYMBOL} "
                "exist passed with the following output:\n"
                "${OUTPUT}\nFile ${_SOURCEFILE_ISTYPE}:\n"
                "${_CHECK_TYPE_SOURCE_ISTYPE}\n")

            # Compiles. This can be a type, or a variable
            file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/${_SOURCEFILE_ISVAR}" "${_CHECK_TYPE_SOURCE_ISVAR}")
            try_compile(${VARIABLE}_ISVAR
                ${CMAKE_BINARY_DIR}
                "${_SOURCEFILE_ISVAR}"
                COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
                CMAKE_FLAGS
                -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_TYPE_EXISTS_FLAGS}
                "${CMAKE_TYPE_EXISTS_INCLUDES}"
                OUTPUT_VARIABLE OUTPUT)
            if(${VARIABLE}_ISVAR)
                if(NOT user_CMAKE_REQUIRED_QUIET)
                    message(STATUS "Looking for type ${SYMBOL} - not found (found variable instead)")
                endif()
                set(${VARIABLE} "" CACHE INTERNAL "Have type ${SYMBOL}")
                file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
                    "Determining if the ${SYMBOL} "
                    "exist failed with the following output:\n"
                    "${OUTPUT}\nFile ${_SOURCEFILE_ISVAR}:\n"
                    "${_CHECK_TYPE_SOURCE_ISVAR}\n")
            else()
                if(NOT user_CMAKE_REQUIRED_QUIET)
                    message(STATUS "Looking for type ${SYMBOL} - found")
                endif()
                set(${VARIABLE} 1 CACHE INTERNAL "Have type ${SYMBOL}")
                file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
                    "Determining if the ${SYMBOL} "
                    "exist passed with the following output:\n"
                    "${OUTPUT}\nFile ${_SOURCEFILE_ISVAR}:\n"
                    "${_CHECK_TYPE_SOURCE_ISVAR}\n")
            endif()
            unset(${VARIABLE}_ISVAR)
        else()
            if(NOT user_CMAKE_REQUIRED_QUIET)
                message(STATUS "Looking for type ${SYMBOL} - not found")
            endif()
            set(${VARIABLE} "" CACHE INTERNAL "Have type ${SYMBOL}")
            file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
                "Determining if the ${SYMBOL} "
                "exist failed with the following output:\n"
                "${OUTPUT}\nFile ${_SOURCEFILE_ISTYPE}:\n"
                "${_CHECK_TYPE_SOURCE_ISTYPE}\n")
        endif()
        unset(_CHECK_TYPE_SOURCE_TYPE)
    endif()
endmacro()
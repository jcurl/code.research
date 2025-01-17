# check_mallopt_option_exists()
#
# On output sets the variable if the symbol is found in 'malloc.h'.
#
# SYMBOL - The mallopt symbol to check for existence.
#
# VARIABLE - The symbol to set with the result. Is either TRUE or FALSE if it
# exists.
#
# The internal variable HAVE_MALLOPT_SYMBOL_${SYMBOL} is cached so that the
# execution is only done once.
#
# We can't use `check_symbol_exists`, as under QNX, the symbol is an enum, and
# CMake won't find it. So we have to do a compile test instead.
function(check_mallopt_option_exists SYMBOL VARIABLE)
    if(DEFINED HAVE_MALLOPT_SYMBOL_${SYMBOL})
        set(${VARIABLE} "${HAVE_MALLOPT_SYMBOL_${SYMBOL}}" PARENT_SCOPE)
        return()
    endif()

    set(CMAKE_REQUIRED_QUIET 1)
    message(STATUS "Looking for mallopt ${SYMBOL}")

    if(CMAKE_C_COMPILER_LOADED)
        _check_mallopt_option_impl(CheckMalloptSymbol.c ${SYMBOL} HAVE_MALLOPT_SYMBOL_${SYMBOL})
    elseif(CMAKE_CXX_COMPILER_LOADED)
        _check_mallopt_option_impl(CheckMalloptSymbol.cpp ${SYMBOL} HAVE_MALLOPT_SYMBOL_${SYMBOL})
    endif()

    # The variable ${HAVE_MALLOPT_SYMBOL_${SYMBOL}} is cached by the
    # `try_compile` method in the macro `_check_mallopt_option_impl`.
    if(HAVE_MALLOPT_SYMBOL_${SYMBOL})
        message(STATUS "Looking for mallopt ${SYMBOL} - found")
    else()
        message(STATUS "Looking for mallopt ${SYMBOL} - not found")
    endif()
    set(${VARIABLE} "${HAVE_MALLOPT_SYMBOL_${SYMBOL}}" PARENT_SCOPE)
endfunction(check_mallopt_option_exists)

macro(_check_mallopt_option_impl SOURCEFILE SYMBOL VARIABLE)
    if(NOT DEFINED "${VARIABLE}")
        set(_MALLOPT_SOURCE "/* */\n")
        string(APPEND _MALLOPT_SOURCE "
#include <malloc.h>

int main(int argc, char* argv[]) {
  mallopt(${SYMBOL}, 0);
  return 0;
}
")

        file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/${SOURCEFILE}" "${_MALLOPT_SOURCE}")
        try_compile(${VARIABLE} ${CMAKE_BINARY_DIR} "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/${SOURCEFILE}" OUTPUT_VARIABLE OUTPUT)
        unset(_MALLOPT_SOURCE)
    endif()
endmacro(_check_mallopt_option_impl)

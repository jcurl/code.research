# check_mallopt_option_exists()
#
# On output sets the variable if the symbol is found in 'malloc.h'.
#
# We can't use `check_symbol_exists`, as under QNX, the symbol is an enum, and
# CMake won't find it. So we have to do a compile test instead.
function(check_mallopt_option_exists symbol variable)
    set(CMAKE_REQUIRED_QUIET 1)
    message(STATUS "Looking for mallopt ${symbol}")
    check_c_source_compiles("
#include <malloc.h>

int main(int argc, char* argv[]) {
  mallopt(${symbol}, 0);
  return 0;
}"
        HAVE_MALLOPT_SYMBOL_${symbol}
    )

    if(HAVE_MALLOPT_SYMBOL_${symbol})
        message(STATUS "Looking for mallopt ${symbol} - found")
        set(${variable} HAVE_MALLOPT_SYMBOL_${symbol})
        set(${variable} ${${variable}} PARENT_SCOPE)
    else()
        message(STATUS "Looking for mallopt ${symbol} - not found")
    endif()
endfunction(check_mallopt_option_exists)

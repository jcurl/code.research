# arm64_has_lse()
#
# On output, sets the variables to TRUE only if the compiler can compile the LSE
# instructions. This doesn't indicate that the processor will support the
# instructions however. Cache variables set (TRUE or FALSE) are:
#
# - HAVE_C_ARM64_LSE
# - HAVE_CXX_ARM64_LSE
#
# The two variables are required, because there are separate CMAKE_C_FLAGS and
# CMAKE_CXX_FLAGS that can change the behaviour.
#
# If the system processor is not ARM, then the variables are not touched.
function(arm64_has_lse)
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
        if(DEFINED HAVE_C_ARM64_LSE OR DEFINED HAVE_CXX_ARM64_LSE)
            return()
        endif()

        message(STATUS "Checking for ARM LSE instructions")
        if(CMAKE_C_COMPILER_LOADED)
            _arm64_has_le_impl(CheckHasArmLse.c HAVE_C_ARM64_LSE)
            if(HAVE_C_ARM64_LSE)
                list(APPEND ARM64_HAS_LSE_LANGUAGES "C")
                set(HAVE_C_ARM64_LSE 1 CACHE STRING "If the C compiler can compile AARCH64 LSE instructions")
            else()
                set(HAVE_C_ARM64_LSE "" CACHE STRING "If the C compiler can compile AARCH64 LSE instructions")
            endif()
        endif()
        if(CMAKE_CXX_COMPILER_LOADED)
            _arm64_has_le_impl(CheckHasArmLse.cpp HAVE_CXX_ARM64_LSE)
            if(HAVE_CXX_ARM64_LSE)
                list(APPEND ARM64_HAS_LSE_LANGUAGES "CXX")
                set(HAVE_CXX_ARM64_LSE 1 CACHE STRING "If the CXX compiler can compile AARCH64 LSE instructions")
            else()
                set(HAVE_CXX_ARM64_LSE "" CACHE STRING "If the CXX compiler can compile AARCH64 LSE instructions")
            endif()
        endif()

        if("${ARM64_HAS_LSE_LANGUAGES}" STREQUAL "")
            message(STATUS "Checking for ARM LSE instructions - not found")
        else()
            message(STATUS "Checking for ARM LSE instructions - ${ARM64_HAS_LSE_LANGUAGES}")
        endif()
    endif()
endfunction(arm64_has_lse)

macro(_arm64_has_le_impl SOURCEFILE VARIABLE)
    if(NOT DEFINED "${VARIABLE}")
        set(_LSE_SOURCE "/* */\n")
        string(APPEND _LSE_SOURCE "
int main(int argc, char **argv) {
    asm volatile(
        \"  cas w0, w1, [sp];\"
        :
        :
        : \"w0\", \"w1\");
    return 0;
}
")

        # Inspired by sources for CheckSymbolExists.cmake in CMake-3.16.9
        file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/${SOURCEFILE}" "${_LSE_SOURCE}")
        try_compile(${VARIABLE} ${CMAKE_BINARY_DIR} "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/${SOURCEFILE}" OUTPUT_VARIABLE OUTPUT)
        unset(_LSE_SOURCE)
    endif()
endmacro(_arm64_has_le_impl)

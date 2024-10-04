set(ARM_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})

# arm64_has_lse()
#
# On output, sets the variables to TRUE only if the compiler can compile the LSE instructions.
# This doesn't indicate that the processor will support the instructions however. Cache variables
# set are:
#
#  HAVE_C_ARM64_LSE
#  HAVE_CXX_ARM64_LSE
#
# The two variables are required, because there are separate CMAKE_C_FLAGS and CMAKE_CXX_FLAGS
# that can change the behaviour.
function(arm64_c_has_lse)
    try_compile(result "${CMAKE_BINARY_DIR}"
        SOURCES "${ARM_CMAKE_DIR}/src/arm_lse.c")
    if (result)
        set(HAVE_C_ARM64_LSE TRUE CACHE BOOL "If the C compiler can compile AARCH64 LSE instructions")
    endif()
endfunction()

function(arm64_cxx_has_lse)
    # Because we can't tell `try_compile` which compiler to use for the file, we
    # have to copy the source with a valid extension the project is configured
    # for.
    file(READ "${ARM_CMAKE_DIR}/src/arm_lse.c" arm_lse_source)
    file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/arm_lse.cpp" "${arm_lse_source}\n")

    try_compile(result "${CMAKE_BINARY_DIR}"
        SOURCES "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/arm_lse.cpp")
    if (result)
        set(HAVE_CXX_ARM64_LSE TRUE CACHE BOOL "If the CXX compiler can compile AARCH64 LSE instructions")
    endif()
endfunction()

function(arm64_has_lse)
    if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
        message(STATUS "Checking for ARM LSE instructions")
        get_property(languages GLOBAL PROPERTY ENABLED_LANGUAGES)
        if("C" IN_LIST languages)
            arm64_c_has_lse()
            if(HAVE_C_ARM64_LSE)
                list(APPEND ARM64_HAS_LSE_LANGUAGES "C")
            endif()
        endif()
        if("CXX" IN_LIST languages)
            arm64_cxx_has_lse()
            if(HAVE_CXX_ARM64_LSE)
                list(APPEND ARM64_HAS_LSE_LANGUAGES "CXX")
            endif()
        endif()

        if("${ARM64_HAS_LSE_LANGUAGES}" STREQUAL "")
            message(STATUS "Checking for ARM LSE instructions - not found")
        else()
            message(STATUS "Checking for ARM LSE instructions - ${ARM64_HAS_LSE_LANGUAGES}")
        endif()
    endif()
endfunction(arm64_has_lse)


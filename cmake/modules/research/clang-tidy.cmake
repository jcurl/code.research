macro(check_clang_tidy)
    option(ENABLE_CLANG_TIDY "Enable checks using Clang-Tidy" ON)
    if(ENABLE_CLANG_TIDY)
        if(NOT DEFINED CMAKE_CXX_CLANG_TIDY)
            find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
            if(CLANG_TIDY_EXE)
                set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
                # Must specify the C++17 version explicitly. When using GCC toolchain as
                # the compiler, it might decide that it doesn't need to provide the
                # "std=c++17" option (GCC 11.4 supports by default C++17). But then
                # running "clang-tidy" e.g. from clang toolchain 14.0 needs the option.
                # It isn't given as CMake doesn't do an additional check for clang-tidy.
                # If you compile using clang++, then the two come from the same
                # toolchain, so the assumption that the two are compatible holds.
                #
                # https://stackoverflow.com/questions/46808702/how-to-integrate-clang-tidy-with-cmake-lang-clang-tidy-and-msvc
                # https://gitlab.kitware.com/cmake/cmake/-/issues/24238
                if(CMAKE_CROSSCOMPILING)
                    message(STATUS "LLVM clang-tidy is disabled (cross compiling not supported)")
                else()
                    if(CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang")
                        message(STATUS "LLVM clang-tidy is enabled with -DENABLE_CLANG_TIDY=on")
                        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
                    else()
                        message(STATUS "LLVM clang-tidy is enabled with -DENABLE_CLANG_TIDY=on ensuring C++17")
                        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--extra-arg-before=-std=c++17")
                    endif()
                endif()
            else()
                message(STATUS "LLVM clang-tidy is disabled (not found)")
            endif()
        else()
            if(CMAKE_CXX_CLANG_TIDY STREQUAL "")
                message(STATUS "LLVM clang-tidy is disabled with CMAKE_CXX_CLANG_TIDY variable")
            else()
                message(STATUS "LLVM clang-tidy is enabled with CMAKE_CXX_CLANG_TIDY variable")
            endif()
        endif()
    else()
        message(STATUS "LLVM clang-tidy is disabled with -DENABLE_CLANG_TIDY=off")
        unset(CMAKE_CXX_CLANG_TIDY)
    endif()
endmacro(check_clang_tidy)

# _CMAKE_CLANG_TIDY_FIND_VERSION()
#
# Sets the internal cache variable `_CMAKE_CLANG_TIDY_VERSION` to the version of
# clang-tidy reported by `${CLANG_TIDY_COMMAND} --version`. It is set to an
# empty string if clang-tidy is not found or `CLANG_TIDY_COMMAND` is empty.
#
# If `_CMAKE_CLANG_TIDY_VERSION` is already defined, nothing is probed.
#
# `CMAKE_REQUIRED_QUIET`: execute quietly without messages.
macro(_CMAKE_CLANG_TIDY_FIND_VERSION)
    if(NOT DEFINED _CMAKE_CLANG_TIDY_VERSION)
        if(NOT CMAKE_REQUIRED_QUIET)
            message(STATUS "Looking for clang-tidy version...")
        endif()

        set(_CMAKE_CLANG_TIDY_VERSION_FOUND "")
        if(CLANG_TIDY_COMMAND)
            execute_process(
                COMMAND "${CLANG_TIDY_COMMAND}" --version
                OUTPUT_VARIABLE _CMAKE_CLANG_TIDY_VERSION_OUTPUT
                ERROR_QUIET
                RESULT_VARIABLE _CMAKE_CLANG_TIDY_VERSION_RESULT
            )
            if(_CMAKE_CLANG_TIDY_VERSION_RESULT EQUAL 0 AND
               _CMAKE_CLANG_TIDY_VERSION_OUTPUT MATCHES "version ([0-9]+(\\.[0-9]+)*)")
                set(_CMAKE_CLANG_TIDY_VERSION_FOUND "${CMAKE_MATCH_1}")
            endif()
        endif()

        set(_CMAKE_CLANG_TIDY_VERSION "${_CMAKE_CLANG_TIDY_VERSION_FOUND}" CACHE INTERNAL "Version of clang-tidy")

        if(NOT CMAKE_REQUIRED_QUIET)
            if(_CMAKE_CLANG_TIDY_VERSION)
                message(STATUS "Looking for clang-tidy version... Found ${_CMAKE_CLANG_TIDY_VERSION}")
            else()
                message(STATUS "Looking for clang-tidy version... clang-tidy not found")
            endif()
        endif()

        unset(_CMAKE_CLANG_TIDY_VERSION_FOUND)
        unset(_CMAKE_CLANG_TIDY_VERSION_OUTPUT)
        unset(_CMAKE_CLANG_TIDY_VERSION_RESULT)
    endif()
endmacro(_CMAKE_CLANG_TIDY_FIND_VERSION)

# _CMAKE_CLANG_TIDY_FIND_CONFIG()
#
# Sets the internal cache variable `_CMAKE_CLANG_TIDY_CONFIG` to the path of the
# clang-tidy configuration file `.clang-tidy-<version>` to use. The file with
# the highest `<version>` that is not greater than `_CMAKE_CLANG_TIDY_VERSION`
# wins. Directories are searched from `CMAKE_CURRENT_SOURCE_DIR` up to
# `PROJECT_SOURCE_DIR`, and the nearest directory wins on equal versions.
#
# It is set to an empty string if no such file exists, or if
# `_CMAKE_CLANG_TIDY_VERSION` is empty, so clang-tidy uses its defaults.
#
# If `_CMAKE_CLANG_TIDY_CONFIG` is already defined, nothing is searched.
#
# `CMAKE_REQUIRED_QUIET`: execute quietly without messages.
macro(_CMAKE_CLANG_TIDY_FIND_CONFIG)
    if(NOT DEFINED _CMAKE_CLANG_TIDY_CONFIG)
        set(_CMAKE_CLANG_TIDY_CONFIG_FOUND "")
        if(_CMAKE_CLANG_TIDY_VERSION)
            if(NOT CMAKE_REQUIRED_QUIET)
                message(STATUS "Looking for clang-tidy configuration file...")
            endif()

            set(_CMAKE_CLANG_TIDY_CONFIG_BEST_VERSION "")
            set(_CMAKE_CLANG_TIDY_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

            # Only walk up when the project directory is an ancestor of the current
            # directory. Otherwise search just the current directory.
            string(FIND "${CMAKE_CURRENT_SOURCE_DIR}/" "${PROJECT_SOURCE_DIR}/" _CMAKE_CLANG_TIDY_CONFIG_POS)
            set(_CMAKE_CLANG_TIDY_CONFIG_DONE FALSE)

            while(NOT _CMAKE_CLANG_TIDY_CONFIG_DONE)
                file(GLOB _CMAKE_CLANG_TIDY_CONFIG_FILES "${_CMAKE_CLANG_TIDY_CONFIG_DIR}/.clang-tidy-*")
                foreach(_CMAKE_CLANG_TIDY_CONFIG_FILE IN LISTS _CMAKE_CLANG_TIDY_CONFIG_FILES)
                    get_filename_component(_CMAKE_CLANG_TIDY_CONFIG_NAME "${_CMAKE_CLANG_TIDY_CONFIG_FILE}" NAME)
                    if(_CMAKE_CLANG_TIDY_CONFIG_NAME MATCHES "^\\.clang-tidy-([0-9]+(\\.[0-9]+)*)$" AND
                       NOT IS_DIRECTORY "${_CMAKE_CLANG_TIDY_CONFIG_FILE}")
                        set(_CMAKE_CLANG_TIDY_CONFIG_CANDIDATE "${CMAKE_MATCH_1}")
                        if(_CMAKE_CLANG_TIDY_CONFIG_CANDIDATE VERSION_LESS_EQUAL _CMAKE_CLANG_TIDY_VERSION AND
                           (NOT _CMAKE_CLANG_TIDY_CONFIG_FOUND OR
                            _CMAKE_CLANG_TIDY_CONFIG_CANDIDATE VERSION_GREATER _CMAKE_CLANG_TIDY_CONFIG_BEST_VERSION))
                            set(_CMAKE_CLANG_TIDY_CONFIG_BEST_VERSION "${_CMAKE_CLANG_TIDY_CONFIG_CANDIDATE}")
                            set(_CMAKE_CLANG_TIDY_CONFIG_FOUND "${_CMAKE_CLANG_TIDY_CONFIG_FILE}")
                        endif()
                    endif()
                endforeach()

                if(NOT _CMAKE_CLANG_TIDY_CONFIG_POS EQUAL 0 OR
                   "${_CMAKE_CLANG_TIDY_CONFIG_DIR}" STREQUAL "${PROJECT_SOURCE_DIR}")
                    set(_CMAKE_CLANG_TIDY_CONFIG_DONE TRUE)
                else()
                    get_filename_component(_CMAKE_CLANG_TIDY_CONFIG_PARENT "${_CMAKE_CLANG_TIDY_CONFIG_DIR}" DIRECTORY)
                    if("${_CMAKE_CLANG_TIDY_CONFIG_PARENT}" STREQUAL "${_CMAKE_CLANG_TIDY_CONFIG_DIR}")
                        set(_CMAKE_CLANG_TIDY_CONFIG_DONE TRUE)
                    else()
                        set(_CMAKE_CLANG_TIDY_CONFIG_DIR "${_CMAKE_CLANG_TIDY_CONFIG_PARENT}")
                    endif()
                endif()
            endwhile()

            if(NOT CMAKE_REQUIRED_QUIET)
                if(_CMAKE_CLANG_TIDY_CONFIG_FOUND)
                    message(STATUS "Looking for clang-tidy configuration file... ${_CMAKE_CLANG_TIDY_CONFIG_FOUND}")
                else()
                    message(STATUS "Looking for clang-tidy configuration file... using clang-tidy defaults")
                endif()
            endif()

            unset(_CMAKE_CLANG_TIDY_CONFIG_BEST_VERSION)
            unset(_CMAKE_CLANG_TIDY_CONFIG_DIR)
            unset(_CMAKE_CLANG_TIDY_CONFIG_POS)
            unset(_CMAKE_CLANG_TIDY_CONFIG_DONE)
            unset(_CMAKE_CLANG_TIDY_CONFIG_FILES)
            unset(_CMAKE_CLANG_TIDY_CONFIG_FILE)
            unset(_CMAKE_CLANG_TIDY_CONFIG_NAME)
            unset(_CMAKE_CLANG_TIDY_CONFIG_CANDIDATE)
            unset(_CMAKE_CLANG_TIDY_CONFIG_PARENT)
        endif()

        set(_CMAKE_CLANG_TIDY_CONFIG "${_CMAKE_CLANG_TIDY_CONFIG_FOUND}" CACHE INTERNAL "Path of clang-tidy configuration file")
        unset(_CMAKE_CLANG_TIDY_CONFIG_FOUND)
    endif()
endmacro(_CMAKE_CLANG_TIDY_FIND_CONFIG)

# check_clang_tidy()
#
# Enables clang-tidy integration for the current project when the
# `CMAKE_RUN_CLANG_TIDY` option is enabled. It searches for a clang-tidy binary
# via `find_program(CLANG_TIDY_COMMAND NAMES "clang-tidy")`, uses the internal
# helper macros to determine the detected version and an optional matching
# `.clang-tidy-<version>` config file, and exports the compile database so the
# clang-tidy driver can run.
#
# If a matching configuration file is found, it is passed to clang-tidy as
# `--config-file=<path>`. Otherwise the default behavioru is left unchanged and
# clang-tidy uses its built-in defaults (and defaults to `.clang-tidy`).
#
# This macro also refuses to redefine CMAKE_C_CLANG_TIDY / CMAKE_CXX_CLANG_TIDY
# when they are already set for the project.
macro(check_clang_tidy)
    option(CMAKE_RUN_CLANG_TIDY "Enable checks using Clang-Tidy" ON)
    if(NOT CMAKE_C_COMPILER_LOADED AND NOT CMAKE_CXX_COMPILER_LOADED)
        message(FATAL "Clang-tidy checks enabled require C/C++ project set")
    endif()
    if(CMAKE_RUN_CLANG_TIDY)
        set(_CMAKE_RUN_CLANG_TIDY_CHECK 1)
        if(CMAKE_C_COMPILER_LOADED AND DEFINED CMAKE_C_CLANG_TIDY)
            set(_CMAKE_RUN_CLANG_TIDY_CHECK 0)
        endif()
        if(CMAKE_CXX_COMPILER_LOADED AND DEFINED CMAKE_CXX_CLANG_TIDY)
            set(_CMAKE_RUN_CLANG_TIDY_CHECK 0)
        endif()

        if(_CMAKE_RUN_CLANG_TIDY_CHECK)
            find_program(CLANG_TIDY_COMMAND NAMES "clang-tidy")
            if(CLANG_TIDY_COMMAND)
                _CMAKE_CLANG_TIDY_FIND_VERSION()
                _CMAKE_CLANG_TIDY_FIND_CONFIG()
                if(DEFINED _CMAKE_CLANG_TIDY_CONFIG AND _CMAKE_CLANG_TIDY_CONFIG)
                    set(_CMAKE_CLANG_TIDY_CONFIG_OPT ";--config-file=${_CMAKE_CLANG_TIDY_CONFIG}")
                endif()

                set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
                if(CMAKE_C_COMPILER_LOADED)
                    if(NOT CMAKE_CROSSCOMPILING)
                        if(CMAKE_C_COMPILER_ID MATCHES "[Cc]lang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
                            message(STATUS "LLVM clang-tidy for C enabled")
                            set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_COMMAND}${_CMAKE_CLANG_TIDY_CONFIG_OPT}")
                        else()
                            message(STATUS "LLVM clang-tidy disabled (unknown C compiler ${CMAKE_C_COMPILER_ID})")
                        endif()
                    else()
                        if(CMAKE_C_COMPILER_ID MATCHES "[Cc]lang")
                            message(STATUS "LLVM clang-tidy for C enabled (cross-compiling with clang)")
                        else()
                            message(STATUS "LLVM clang-tidy disabled (cross-compiling non-clang C toolchain ${CMAKE_C_COMPILER_ID})")
                        endif()
                    endif()
                endif()

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
                if(CMAKE_CXX_COMPILER_LOADED)
                    if(NOT CMAKE_CROSSCOMPILING)
                        if(CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang")
                            message(STATUS "LLVM clang-tidy for C++ enabled")
                            set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}${_CMAKE_CLANG_TIDY_CONFIG_OPT}")
                        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
                            message(STATUS "LLVM clang-tidy for C++ enabled, ensuring C++17 for GNU")
                            set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}${_CMAKE_CLANG_TIDY_CONFIG_OPT};--extra-arg-before=-std=c++17")
                        else()
                            message(STATUS "LLVM clang-tidy disabled (unknown C++ compiler ${CMAKE_CXX_COMPILER_ID})")
                        endif()
                    else()
                        if(CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang")
                            message(STATUS "LLVM clang-tidy for C++ enabled (cross-compiling with clang)")
                            set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}${_CMAKE_CLANG_TIDY_CONFIG_OPT}")
                        else()
                            message(STATUS "LLVM clang-tidy disabled (cross-compiling non-clang C++ toolchain ${CMAKE_CXX_COMPILER_ID})")
                        endif()
                    endif()
                endif()
            else()
                message(STATUS "LLVM clang-tidy disabled (clang-tidy not found, set CMAKE_CXX_CLANG_TIDY path)")
            endif()
        else()
            if(CMAKE_CXX_COMPILER_LOADED AND NOT CMAKE_CXX_CLANG_TIDY AND
               CMAKE_C_COMPILER_LOADED AND NOT CMAKE_C_CLANG_TIDY)
                message(STATUS "LLVM clang-tidy disabled with CMAKE_[C|CXX]_CLANG_TIDY variable")
            elseif(CMAKE_CXX_COMPILER_LOADED AND CMAKE_CXX_CLANG_TIDY AND
                   CMAKE_C_COMPILER_LOADED AND CMAKE_C_CLANG_TIDY)
                message(STATUS "LLVM clang-tidy enabled with CMAKE_[C|CXX]_CLANG_TIDY variable")
            elseif(CMAKE_CXX_COMPILER_LOADED AND CMAKE_CXX_CLANG_TIDY)
                message(STATUS "LLVM clang-tidy enabled with CMAKE_CXX_CLANG_TIDY variable")
            elseif(CMAKE_C_COMPILER_LOADED AND CMAKE_C_CLANG_TIDY)
                message(STATUS "LLVM clang-tidy enabled with CMAKE_C_CLANG_TIDY variable")
            else()
                message(STATUS "LLVM clang-tidy enabled")
            endif()
        endif()
        unset(_CMAKE_RUN_CLANG_TIDY_CHECK)
    else()
        message(STATUS "LLVM clang-tidy is disabled with -DCMAKE_RUN_CLANG_TIDY=off")
        if(CMAKE_CXX_COMPILER_LOADED)
            unset(CMAKE_CXX_CLANG_TIDY)
        endif()
        if(CMAKE_C_COMPILER_LOADED)
            unset(CMAKE_C_CLANG_TIDY)
        endif()
    endif()
endmacro(check_clang_tidy)

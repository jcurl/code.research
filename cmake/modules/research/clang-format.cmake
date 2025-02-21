function(_target_clangformat TARGET)
    if(NOT DEFINED CLANGFORMAT_FOUND)
        # A toolchain file can set this variable, which we'll use instead of the
        # default of `clang-format`.
        if(NOT CLANGFORMAT_EXECUTABLE)
            set(CLANGFORMAT_EXECUTABLE clang-format)
        endif()

        if(NOT EXISTS ${CLANGFORMAT_EXECUTABLE})
            find_program(_clangformat_executable ${CLANGFORMAT_EXECUTABLE})
            if(_clangformat_executable)
                set(CLANGFORMAT_FOUND TRUE CACHE INTERNAL "TRUE if clang-format is found")
                set(CLANGFORMAT_EXECUTABLE ${_clangformat_executable} CACHE FILEPATH "clang-format path of executable")
            else()
                # If it's not found, we report it to the user, but don't abort
                # the build. It's not critical for users that build.
                message(STATUS "ClangFormat: ${CLANGFORMAT_EXECUTABLE} not found!")
                set(CLANGFORMAT_FOUND FALSE CACHE INTERNAL "TRUE if clang-format is found")
            endif()
            unset(_clangformat_executable CACHE)
        endif()
    endif()

    if(CLANGFORMAT_FOUND)
        _clangformat_version()
        if(CLANGFORMAT_VERSION_FOUND)
            foreach(_clangformat_source ${ARGN})
                # The command ${TARGET}_clangformat runs in the current source
                # directory. So we don't need to extend the path names, making
                # the total path shorter.

                #get_filename_component(_clangformat_source ${_clangformat_source} ABSOLUTE)
                list(APPEND _clangformat_sources ${_clangformat_source})
            endforeach()

            # Don't add twice, allowing the user to incrementally add files.
            if(TARGET ${TARGET}_clangformat)
                message(FATAL_ERROR "target_clangformat() can only be called once per target")
            endif()

            add_custom_target(${TARGET}_clangformat
                COMMAND ${CLANGFORMAT_EXECUTABLE} -i ${_clangformat_sources}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                COMMENT "Formatting ${TARGET} with ${CLANGFORMAT_EXECUTABLE} ..."
            )

            if(TARGET clangformat)
                add_dependencies(clangformat ${TARGET}_clangformat)
            else()
                add_custom_target(clangformat DEPENDS ${TARGET}_clangformat)
            endif()
        endif()
    endif()
endfunction()

function(_clangformat_version)
    get_property(CLANGFORMAT_MIN_VERSION_REQUIRED GLOBAL PROPERTY CLANGFORMAT_MIN_VERSION_REQUIRED)
    if(CLANGFORMAT_MIN_VERSION_REQUIRED)
        if(NOT DEFINED CLANGFORMAT_VERSION)
            execute_process(
                COMMAND clang-format --version
                OUTPUT_VARIABLE _CLANGFORMAT_VERSION_RAW_OUTPUT)

            string(REGEX MATCH "clang-format version ([0-9]+\.[0-9]+(\.[0-9]+)?)" _CLANGFORMAT_VERSION "${_CLANGFORMAT_VERSION_RAW_OUTPUT}")
            if("${_CLANGFORMAT_VERSION}" STREQUAL "")
                set(CLANGFORMAT_VERSION "" CACHE INTERNAL "Found version of clang-format")
                set(CLANGFORMAT_VERSION_FOUND FALSE PARENT_SCOPE)
                message(STATUS "clang-format: UNKNOWN (minimum required ${CLANGFORMAT_MIN_VERSION_REQUIRED})")
                return()
            endif()

            set(CLANGFORMAT_VERSION "${CMAKE_MATCH_1}" CACHE INTERNAL "Found version of clang-format")
            set_property(GLOBAL PROPERTY CLANGFORMAT_VERSION ${CLANGFORMAT_VERSION})
        else()
            set(CMAKE_REQUIRED_QUIET 1)
        endif()

        set(CLANGFORMAT_VERSION ${CLANGFORMAT_VERSION} CACHE INTERNAL "Found version of clang-format")
        if(CLANGFORMAT_VERSION VERSION_GREATER_EQUAL CLANGFORMAT_MIN_VERSION_REQUIRED)
            set(CLANGFORMAT_VERSION_FOUND TRUE PARENT_SCOPE)
            if(NOT CMAKE_REQUIRED_QUIET)
                message(STATUS "clang-format: ${CLANGFORMAT_VERSION} (minimum required ${CLANGFORMAT_MIN_VERSION_REQUIRED})")
            endif()
        else()
            set(CLANGFORMAT_VERSION_FOUND FALSE PARENT_SCOPE)
            if(NOT CMAKE_REQUIRED_QUIET)
                message(STATUS "clang-format: ${CLANGFORMAT_VERSION} not usable (minimum required ${CLANGFORMAT_MIN_VERSION_REQUIRED})")
            endif()
        endif()
    else()
        # The user didn't specify a version with `target_clangformat(VERSION
        # version)`, so accept all.
        set(CLANGFORMAT_VERSION_FOUND TRUE PARENT_SCOPE)
    endif()
endfunction()

# If you pass a list as DIR, EXTENSIONS, SOURCES, then ensure it's in quotes,
# like "${arg_DIR}".
function(_file_glob RECURSE DIR EXTENSIONS SOURCES)
    set(_sources ${${SOURCES}})
    foreach(_extension ${EXTENSIONS})
        foreach(_dir ${DIR})
            if(RECURSE)
                file(GLOB_RECURSE _files LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${_dir}/*.${_extension}")
            else()
                file(GLOB _files LIST_DIRECTORIES false RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${_dir}/*.${_extension}")
            endif()
            list(APPEND _sources ${_files})
        endforeach()
    endforeach()
    set(${SOURCES} ${_sources} PARENT_SCOPE)
endfunction()

# target_clangformat([VERSION version] TARGET ${BINARY} [DIR .] [SOURCE list])
#
# One can request the minimum version of clang-format required without any other
# options. The check isn't made until a target rule needs to be created.
#
# `TARGET`: Provide the target that was usually previously specified with
# `add_executable` or `add_library` or `add_test`. A rule will be created
# `${TARGET}_clangformat`.
#
# If the `TARGET` is used alone, then the sources given by the target are used.
# The target must exist, as the sources from this target at the time of
# invocation are used.
#
# If `DIR .` is given, then all files in the directory specified are used (the
# sources in the target are not used). This is a file glob, and sources are
# included based on the project language defined (C or CXX). The TARGET can be a
# non-existing target.
#
# Similarly, if `DIR_RECURSE .` is given, then all files in the directory, and
# all subdirectories are used, like `DIR`.
#
# If `SOURCE file` is given, then the list of files specified are used. The
# TARGET can be a non-existing target.
#
# This function will fail if the same target is specified multiple times.
#
# Making the target `clangformat` will make all the targets already specified.
function(target_clangformat)
    set(_singleargs TARGET VERSION)
    set(_multiargs SOURCE DIR DIR_RECURSE)
    cmake_parse_arguments(PARSE_ARGV 0 arg "" "${_singleargs}" "${_multiargs}")
    if(arg_VERSION)
        # The minimum version we need. This will be checked at the time we do
        # the search. So the search is lazy.
        set_property(GLOBAL PROPERTY CLANGFORMAT_MIN_VERSION_REQUIRED ${arg_VERSION})
    endif()
    if(arg_TARGET)
        if(arg_DIR)
            if(CMAKE_C_COMPILER_LOADED)
                _file_glob(FALSE "${arg_DIR}" "c;h" _target_sources)
            endif()
            if(CMAKE_CXX_COMPILER_LOADED)
                _file_glob(FALSE "${arg_DIR}" "cpp;hpp;h;cxx;hxx;c++;h++;cc;hh" _target_sources)
            endif()
            list(REMOVE_DUPLICATES _target_sources)
        endif()
        if(arg_DIR_RECURSE)
            if(CMAKE_C_COMPILER_LOADED)
                _file_glob(TRUE "${arg_DIR_RECURSE}" "c;h" _target_sources)
            endif()
            if(CMAKE_CXX_COMPILER_LOADED)
                _file_glob(TRUE "${arg_DIR_RECURSE}" "cpp;hpp;h;cxx;hxx;c++;h++;cc;hh" _target_sources)
            endif()
            list(REMOVE_DUPLICATES _target_sources)
        endif()
        if(arg_SOURCE)
            list(APPEND _target_sources ${arg_SOURCE})
            list(REMOVE_DUPLICATES _target_sources)
        endif()
        if(NOT DEFINED arg_DIR AND NOT DEFINED arg_DIR_RECURSE AND NOT DEFINED arg_SOURCE)
            get_target_property(_target_sources ${arg_TARGET} SOURCES)
        endif()
        if(_target_sources)
            _target_clangformat(${arg_TARGET} ${_target_sources})
        endif()
    endif()
endfunction()

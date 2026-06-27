# cmake_git_check()
#
# Run automatically when this module is imported. If the variable ${CMAKE_GIT}
# is not defined, it queries the current git commit and if it is dirty. It
# places the result in ${CMAKE_GIT}. This variable is used later in other macros
# to add version information from the build.
#
# See also `qnx.cmake`
macro(cmake_git_check)
    if(NOT DEFINED CMAKE_GIT)
        find_package(Git QUIET)

        if(GIT_FOUND)
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                RESULT_VARIABLE _cmake_git_rev_parse_result
                OUTPUT_VARIABLE _cmake_git_short_sha
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )

            if(_cmake_git_rev_parse_result EQUAL 0 AND _cmake_git_short_sha)
                execute_process(
                    COMMAND "${GIT_EXECUTABLE}" status --porcelain
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                    RESULT_VARIABLE _cmake_git_status_result
                    OUTPUT_VARIABLE _cmake_git_status_output
                    ERROR_QUIET
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )

                if(_cmake_git_status_result EQUAL 0)
                    if(_cmake_git_status_output)
                        set(CMAKE_GIT "${_cmake_git_short_sha}-dirty")
                    else()
                        set(CMAKE_GIT "${_cmake_git_short_sha}")
                    endif()
                    MESSAGE(STATUS "GIT version found: ${CMAKE_GIT}")
                endif()
            endif()

            unset(_cmake_git_rev_parse_result)
            unset(_cmake_git_status_result)
            unset(_cmake_git_short_sha)
            unset(_cmake_git_status_output)
        endif()
    endif()
endmacro()

cmake_git_check()

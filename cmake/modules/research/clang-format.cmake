# https://github.com/zemasoft/clangformat-cmake/blob/ced236e1919c412f5cf745de9a116cae119e7315/cmake/ClangFormat.cmake

# Copyright Tomas Zeman 2019-2020.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

# Modified from original

function(prefix_clangformat_setup prefix)
  # Only do the search once, so we warn only once.
  get_property(clangformat_found GLOBAL PROPERTY CLANGFORMAT_FOUND)
  if(NOT DEFINED clangformat_found)
    if(NOT CLANGFORMAT_EXECUTABLE)
      set(CLANGFORMAT_EXECUTABLE clang-format)
    endif()

    if(NOT EXISTS ${CLANGFORMAT_EXECUTABLE})
      find_program(clangformat_executable_tmp ${CLANGFORMAT_EXECUTABLE})
      if(clangformat_executable_tmp)
        set(CLANGFORMAT_EXECUTABLE ${clangformat_executable_tmp})
        unset(clangformat_executable_tmp)
        set_property(GLOBAL PROPERTY CLANGFORMAT_FOUND TRUE)
        set_property(GLOBAL PROPERTY CLANGFORMAT_EXECUTABLE ${clangformat_executable_tmp})
      else()
        # If it's not found, we report it to the user, but don't abort the
        # build. It's not critical for users that build.
        message(STATUS "ClangFormat: ${CLANGFORMAT_EXECUTABLE} not found!")
        set_property(GLOBAL PROPERTY CLANGFORMAT_FOUND FALSE)
      endif()
    endif()
  else()
    if(clangformat_found MATCHES "TRUE")
      get_property(clangformat_exe GLOBAL PROPERTY CLANGFORMAT_EXECUTABLE)
      set(CLANGFORMAT_EXECUTABLE ${clangformat_exe})
    endif()
  endif()

  if(DEFINED CLANGFORMAT_EXECUTABLE)
    foreach(clangformat_source ${ARGN})
      get_filename_component(clangformat_source ${clangformat_source} ABSOLUTE)
      list(APPEND clangformat_sources ${clangformat_source})
    endforeach()

    add_custom_target(${prefix}_clangformat
      COMMAND
        ${CLANGFORMAT_EXECUTABLE}
        -i
        ${clangformat_sources}
      WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}
      COMMENT
        "Formatting ${prefix} with ${CLANGFORMAT_EXECUTABLE} ..."
    )

    if(TARGET clangformat)
      add_dependencies(clangformat ${prefix}_clangformat)
    else()
      add_custom_target(clangformat DEPENDS ${prefix}_clangformat)
    endif()
  endif()
endfunction()

function(clangformat_setup)
  prefix_clangformat_setup(${PROJECT_NAME} ${ARGN})
endfunction()

function(target_clangformat_setup target)
  get_target_property(target_sources ${target} SOURCES)
  prefix_clangformat_setup(${target} ${target_sources})
endfunction()

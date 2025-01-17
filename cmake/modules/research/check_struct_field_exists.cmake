include(CheckTypeSize)

# check_struct_field_exists(struct field files variable)
#
# Check if the structure <struct> has the field <field> defined by the include
# files <files>. If it is found, set <variable>.
#
# - CMAKE_REQUIRED_QUIET: If set to TRUE, the messages are not printed.
function(check_struct_field_exists STRUCT FIELD FILES VARIABLE)
    if(NOT DEFINED STRUCT OR "x${STRUCT}" STREQUAL "x")
        message(FATAL_ERROR "check_struct_field_exists: STRUCT not defined")
    endif()
    if(NOT "${STRUCT}" MATCHES "^[a-zA-Z_][a-zA-Z0-9_]*$")
        message(FATAL_ERROR "check_struct_field_exists: STRUCT is an invalid symbol")
    endif()
    if(NOT DEFINED FIELD OR "x${FIELD}" STREQUAL "x")
        message(FATAL_ERROR "check_struct_field_exists: FIELD not defined")
    endif()
    if(NOT "${FIELD}" MATCHES "^[a-zA-Z_][a-zA-Z0-9_]*$")
        message(FATAL_ERROR "check_struct_field_exists: FIELD is an invalid symbol")
    endif()

    set(user_CMAKE_REQUIRED_QUIET "${CMAKE_REQUIRED_QUIET}")
    set(CMAKE_REQUIRED_QUIET 1)

    if(NOT user_CMAKE_REQUIRED_QUIET)
        message(STATUS "Looking for struct ${STRUCT}.${FIELD}")
    endif()

    set(CMAKE_EXTRA_INCLUDE_FILES ${FILES})
    check_type_size("((struct ${STRUCT}*)0)->${FIELD}" ${VARIABLE})

    if(NOT user_CMAKE_REQUIRED_QUIET)
        if(${VARIABLE})
            message(STATUS "Looking for struct ${STRUCT}.${FIELD} - found")
        else()
            message(STATUS "Looking for struct ${STRUCT}.${FIELD} - not found")
        endif()
    endif()
endfunction()

include(GoogleTest)
include(research/check_type_exists)

set(STRINTERN_BINARY str_intern)
set(STRINTERN_SOURCES
    str_intern.cpp str_intern.h
    allocator.cpp allocator.h
    options.cpp options.h
    readbuff.cpp readbuff.h
    intern_forward_list.cpp
    intern_none.cpp
    intern_set.cpp
    intern_fixedset.cpp
    intern_varset.cpp
    intern_varset_pmr.cpp
)

add_executable(${STRINTERN_BINARY} ${STRINTERN_SOURCES})
target_compile_features(${STRINTERN_BINARY} PRIVATE cxx_std_17)
target_link_libraries(${STRINTERN_BINARY} ubench)

check_type_exists("std::pmr::memory_resource" "memory_resource" HAVE_CXX_MEMORY_RESOURCE)
if(NOT HAVE_CXX_MEMORY_RESOURCE)
    check_type_exists("std::experimental::pmr::memory_resource" "experimental/memory_resource" HAVE_CXX_EXPERIMENTAL_MEMORY_RESOURCE)
    if(NOT HAVE_CXX_EXPERIMENTAL_MEMORY_RESOURCE)
        message(FATAL_ERROR "No PMR resources available")
    endif()
endif()

if (QNXNTO AND CMAKE_SYSTEM_VERSION VERSION_LESS 8.0)
    # QNX 8.0 and later automatically include the std::filesystem extensions
    target_link_libraries(${STRINTERN_BINARY} c++fs)
endif()

if(IS_DEBUG)
    add_sanitizers(${STRINTERN_BINARY})
endif()

target_clangformat(TARGET ${STRINTERN_BINARY} DIR .)
target_use_msg(${STRINTERN_BINARY} str_intern.use DESCRIPTION "String intern benchmark")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

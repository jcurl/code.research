include(GoogleTest)

set(STRINTERN_BINARY str_intern)
set(STRINTERN_SOURCES
    str_intern.cpp str_intern.h
    allocator.cpp allocator.h
    options.cpp options.h
    readbuff.cpp readbuff.h
    intern_forward_list.cpp
    intern_none.cpp
    intern_set.cpp
)

add_executable(${STRINTERN_BINARY} ${STRINTERN_SOURCES})
target_compile_features(${STRINTERN_BINARY} PRIVATE cxx_std_17)
target_link_libraries(${STRINTERN_BINARY} ubench)

if (QNXNTO AND CMAKE_SYSTEM_VERSION VERSION_LESS 8.0)
    # QNX 8.0 and later automatically include the std::filesystem extensions
    target_link_libraries(${STRINTERN_BINARY} c++fs)
endif()

if(IS_DEBUG)
    add_sanitizers(${STRINTERN_BINARY})
endif()

target_clangformat(TARGET ${STRINTERN_BINARY} DIR .)
target_use_msg(${STRINTERN_BINARY} str_intern.use DESCRIPTION "String intern benchmark")

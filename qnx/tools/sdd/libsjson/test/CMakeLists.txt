include(GoogleTest)

set(BINARY libsjson_test)
set(SOURCES
    json_reader_test.cpp
    json_reader_context_test.cpp
    json_writer_test.cpp
    json_writer_array_test.cpp
    json_writer_object_test.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_include_directories(${BINARY} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_link_libraries(${BINARY} PRIVATE GTest::gtest_main gmock_main libsjson librtest)
add_test(NAME ${BINARY} COMMAND ${BINARY})

if(QNXNTO)
    if(CMAKE_SYSTEM_VERSION VERSION_LESS 8.0)
        # QNX 8.0 and later automatically include the std::filesystem extensions
        target_link_libraries(${BINARY} PRIVATE c++fs)
    endif()
endif()

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_code_coverage(${BINARY}
    LCOV_OPTIONS
        --ignore-errors inconsistent,inconsistent
    EXCLUDE
        ${CMAKE_CURRENT_SOURCE_DIR}/*
    LLVM_EXCLUDE
        external/*
)

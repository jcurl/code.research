include(GoogleTest)

set(BINARY libsjson_test)
set(SOURCES
    json_writer_test.cpp
    json_writer_array_test.cpp
    json_writer_object_test.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE GTest::gtest_main gmock_main libsjson)
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
    EXCLUDE
    ${CMAKE_SOURCE_DIR}/test/*
    ${CMAKE_BINARY_DIR}/build/*
)

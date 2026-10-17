include(GoogleTest)

set(BINARY libunet_test)
set(SOURCES
    net_test.cpp
    udp_test.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE GTest::gtest_main gmock_main libunet librtest)
add_test(NAME ${BINARY} COMMAND ${BINARY})

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

include(GoogleTest)

set(BINARY librtest_test)
set(SOURCES
    optional_test.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE GTest::gtest_main librtest libstdext)
add_test(NAME ${BINARY} COMMAND ${BINARY})

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_code_coverage(${BINARY}
    LCOV_OPTIONS
        --ignore-errors inconsistent,inconsistent
    EXCLUDE
        ${CMAKE_CURRENT_SOURCE_DIR}/*
        external/*
    LLVM_EXCLUDE
        lib/librtest/*
)

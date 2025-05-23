include(GoogleTest)

set(BINARY stdext_test)
set(SOURCES
    expected_base.cpp
    expected_equality.cpp
    expected_monadic.cpp
    expected_noexcept.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE GTest::gtest_main gmock_main)
add_test(NAME ${BINARY} COMMAND ${BINARY})

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_code_coverage(${BINARY}
    EXCLUDE
    ${CMAKE_SOURCE_DIR}/test/*
    ${CMAKE_BINARY_DIR}/build/*
)

target_clangformat(TARGET ${BINARY} DIR .)

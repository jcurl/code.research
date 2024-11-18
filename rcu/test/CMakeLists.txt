include(GoogleTest)

set(BINARY ${CMAKE_PROJECT_NAME}-test)
set(SOURCES
    rcu-test.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE GTest::gtest_main)
add_test(NAME ${BINARY} COMMAND ${BINARY})

if(IS_DEBUG)
    add_sanitizers(rcutest)
endif()

target_code_coverage(${BINARY}
    EXCLUDE
    ${CMAKE_SOURCE_DIR}/test/*
    ${CMAKE_BINARY_DIR}/build/*
)

target_clangformat_setup(${BINARY})

# Access the source directories under test
include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/test)

enable_testing()

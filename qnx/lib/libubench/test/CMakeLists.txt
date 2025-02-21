include(GoogleTest)

set(BINARY ubench_test)
set(SOURCES
    string_test.cpp
    clock_test.cpp
    flags_test.cpp
    net_test.cpp
    os_test.cpp
    rcu_test.cpp
    strlcpy_test.cpp
    thread_test.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE GTest::gtest_main gmock_main ubench)
add_test(NAME ${BINARY} COMMAND ${BINARY})

if(IS_DEBUG)
    add_sanitizers(rcutest)
endif()

target_code_coverage(${BINARY}
    EXCLUDE
    ${CMAKE_SOURCE_DIR}/test/*
    ${CMAKE_BINARY_DIR}/build/*
)

target_clangformat(TARGET ${BINARY} DIR .)

include(GoogleTest)

set(BINARY libubench_test)
set(SOURCES
    clock_test.cpp
    file_test.cpp
    flags_test.cpp
    measure/print_test.cpp
    options_test.cpp
    os_test.cpp
    rcu_test.cpp
    string_test.cpp
    strlcpy_test.cpp
    str_intern_test.cpp
    sync_event_test.cpp
    thread_test.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE GTest::gtest_main gmock_main libubench)
add_test(NAME ${BINARY} COMMAND ${BINARY})

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_code_coverage(${BINARY}
    LCOV_OPTIONS
        --ignore-errors inconsistent,inconsistent
    EXCLUDE
        ${CMAKE_CURRENT_SOURCE_DIR}/*
)

check_include_files(features.h HAVE_FEATURES_H)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
target_include_directories(${BINARY} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})


set(BINARY_GETOPT libubench_getopt_test)
set(SOURCES_GETOPT
    getopt_compat/options.h getopt_compat/options.cpp
    getopt_compat/getopt_test.cpp
)

add_executable(${BINARY_GETOPT} ${SOURCES_GETOPT})
target_compile_features(${BINARY_GETOPT} PRIVATE cxx_std_17)
target_link_libraries(${BINARY_GETOPT} PRIVATE GTest::gtest_main libubench)
# The test case should not run automatically - it will fail with the various implementations,
# or some implementations of libc result in undefined behaviour.
#add_test(NAME ${BINARY_GETOPT} COMMAND ${BINARY_GETOPT})

target_include_directories(${BINARY_GETOPT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

include(GoogleTest)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(NETBENCH_BINARY net_bench)
set(NETBENCH_SOURCES net_bench.cpp)

add_executable(${NETBENCH_BINARY} ${NETBENCH_SOURCES})
target_compile_features(${NETBENCH_BINARY} PRIVATE cxx_std_17)
target_link_libraries(${NETBENCH_BINARY} PRIVATE ubench GTest::gtest_main benchmark::benchmark)

set(RCUBENCH_BINARY rcu_bench)
set(RCUBENCH_SOURCES rcu_bench.cpp)

add_executable(${RCUBENCH_BINARY} ${RCUBENCH_SOURCES})
target_compile_features(${RCUBENCH_BINARY} PRIVATE cxx_std_17)
target_link_libraries(${RCUBENCH_BINARY} PRIVATE ubench Threads::Threads)

if(QNXNTO)
    # Googletest needs `regex`, QNX has this as a separate library.
    target_link_libraries(${NETBENCH_BINARY} PRIVATE regex)
endif()

if(IS_DEBUG)
    add_sanitizers(${NETBENCH_BINARY})
endif()

target_clangformat_setup(${NETBENCH_BINARY})
target_clangformat_setup(${RCUBENCH_BINARY})

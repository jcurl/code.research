include(GoogleTest)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(NETBENCH_BINARY net_bench)
set(NETBENCH_SOURCES net_bench.cpp)

add_executable(${NETBENCH_BINARY} ${NETBENCH_SOURCES})
target_compile_features(${NETBENCH_BINARY} PRIVATE cxx_std_17)
target_link_libraries(${NETBENCH_BINARY} PRIVATE libubench GTest::gtest_main benchmark::benchmark)

if(IS_DEBUG)
    add_sanitizers(${NETBENCH_BINARY})
endif()

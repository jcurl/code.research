include(GoogleTest)

set(BINARY cacheline_bench)
set(SOURCES
    cacheline_bench.cpp
    options.h options.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE GTest::gtest_main)
target_link_libraries(${BINARY} PRIVATE benchmark::benchmark)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat(TARGET ${BINARY} DIR .)
target_use_msg(${BINARY} cacheline_bench.use DESCRIPTION "Google Benchmark measuring strided reads of memory")

install(TARGETS ${BINARY} DESTINATION bin)

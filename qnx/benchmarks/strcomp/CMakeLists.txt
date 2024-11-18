include(GoogleTest)

set(BINARY strcmp_bench)
set(SOURCES strcmp_bench.cpp)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE GTest::gtest_main)
target_link_libraries(${BINARY} PRIVATE benchmark::benchmark)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat_setup(${BINARY})

include_directories(${CMAKE_SOURCE_DIR}/benchmarks/strcomp)

install(TARGETS ${BINARY} DESTINATION bin)

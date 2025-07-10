include(research/arm64_has_lse)

set(BINARY core_latency)
set(SOURCES core_latency.cpp
    benchmark.h
    core_benchmark.h core_benchmark.cpp
    corerw_benchmark.h corerw_benchmark.cpp
    statistics.h statistics.cpp
    arm64.h arm64.cpp
    options.h options.cpp
)

# Sets HAVE_ARM64_LSE
arm64_has_lse()

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${BINARY} PRIVATE Threads::Threads)
target_link_libraries(${BINARY} PRIVATE ubench)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat(TARGET ${BINARY} DIR .)
target_use_msg(${BINARY} core_latency.use DESCRIPTION "measure core-to-core latency for memory read/writes")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS ${BINARY} DESTINATION bin)

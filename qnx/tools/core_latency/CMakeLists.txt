set(BINARY core_latency)
set(SOURCES core_latency.cpp
    sync_event.h sync_event.cpp
    thread_pin.h thread_pin.cpp
    benchmark.h
    core_benchmark.h core_benchmark.cpp
    corerw_benchmark.h corerw_benchmark.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(${BINARY} Threads::Threads)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat_setup(${BINARY})

install(TARGETS ${BINARY} DESTINATION bin)

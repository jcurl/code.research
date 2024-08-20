set(BINARY core_latency)
set(SOURCES core_latency.cpp
    sync_event.h sync_event.cpp
    thread_pin.h thread_pin.cpp
    benchmark.h
    core_benchmark.h core_benchmark.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat_setup(${BINARY})

install(TARGETS ${BINARY} DESTINATION bin)

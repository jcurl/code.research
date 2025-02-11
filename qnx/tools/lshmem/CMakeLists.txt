set(BINARY lshmem)
set(SOURCES
    lshmem.cpp
    options.cpp options.h
    pid_mapping.cpp pid_mapping.h
    shmem_map.cpp shmem_map.h
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} ubench osqnx)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat_setup(${BINARY})

install(TARGETS ${BINARY} DESTINATION bin)

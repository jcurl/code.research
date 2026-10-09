set(BINARY devc-cpuid)
set(SOURCES
    main.cpp
    cpuid-driver.cpp cpuid-driver.h
    options.cpp options.h
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE libubench libosqnx libcpuid)
target_use_msg(${BINARY} devc-cpuid.use DESCRIPTION "Linux CPUID compatible driver")

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

set(LIBRARY libcpuid)
set(SOURCES
    ../include/cpuid/cpuid.h
    cpuid_dev.h cpuid_dev.cpp
    cpuid_native.h cpuid_native.cpp
    cpuidreader_dev.h cpuidreader_dev.cpp
    cpuidreader_native.h
)

add_library(${LIBRARY} STATIC ${SOURCES})
set_target_properties(${LIBRARY} PROPERTIES OUTPUT_NAME "cpuid")
target_compile_features(${LIBRARY} PRIVATE cxx_std_17)
target_include_directories(${LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()

target_clangformat(TARGET ${LIBRARY} DIR_RECURSE . DIR_RECURSE ../include/cpuid)

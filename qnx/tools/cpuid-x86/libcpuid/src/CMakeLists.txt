set(LIBRARY libcpuid)
set(SOURCES
    ../include/cpuid/cpuid.h
    ../include/cpuid/cpuidreader_dev.h
    ../include/cpuid/cpuidreader_native.h
    ../include/cpuid/cpuid_dev.h
    ../include/cpuid/cpuid_native.h
    cpuid.cpp
    cpuid_dev.cpp
    cpuid_native.cpp
    cpuidreader_dev.cpp
    dump_generic.h dump_generic.cpp
    dump_intel.h dump_intel.cpp
    dump_amd.h dump_amd.cpp
)

add_library(${LIBRARY} STATIC ${SOURCES})
set_target_properties(${LIBRARY} PROPERTIES OUTPUT_NAME "cpuid")
target_compile_features(${LIBRARY} PRIVATE cxx_std_17)
target_include_directories(${LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()

target_clangformat(TARGET ${LIBRARY} DIR_RECURSE . DIR_RECURSE ../include/cpuid)

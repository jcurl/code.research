include(research/check_symbol_enum_exists)

set(LIBRARY libcpuid)
set(SOURCES
    ../include/cpuid/cpuid.h
    ../include/cpuid/cpuidreader_cache.h
    ../include/cpuid/cpuidreader_dev.h
    ../include/cpuid/cpuidreader_native.h
    ../include/cpuid/cpuid_cpuctl.h
    ../include/cpuid/cpuid_dev.h
    ../include/cpuid/cpuid_native.h
    cpuid.cpp
    cpuid_dev.cpp
    cpuid_native.cpp
    cpuid_none.cpp
    cpuidreader_cache.cpp
    cpuidreader_dev.cpp
    dump_generic.h dump_generic.cpp
    dump_intel.h dump_intel.cpp
    dump_amd.h dump_amd.cpp
)

add_library(${LIBRARY} STATIC ${SOURCES})
set_target_properties(${LIBRARY} PROPERTIES OUTPUT_NAME "cpuid")
target_compile_features(${LIBRARY} PRIVATE cxx_std_17)
target_include_directories(${LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

check_symbol_enum_exists(CPUCTL_CPUID_COUNT "sys/types.h;sys/cpuctl.h" HAVE_CPUCTL_CPUID_COUNT)
if (HAVE_CPUCTL_CPUID_COUNT)
    # The header file is always present as it is an interface that may be used
    # by all code. The implementation however would provide a default
    # implementation instead.
    #
    # The cpuid_none.cpp has relevant code that replaces this if needed with a
    # default implementation that does nothing.
    target_sources(${LIBRARY} PRIVATE cpuid_cpuctl.cpp)
endif()

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()

target_clangformat(TARGET ${LIBRARY} DIR_RECURSE . DIR_RECURSE ../include/cpuid)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

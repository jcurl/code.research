include(GoogleTest)
include(CheckSymbolExists)
include(research/mallopt)

set(BINARY malloc_bench)
set(SOURCES
    malloc_bench.cpp
    mallopt.h mallopt.cpp
    allocator.h allocator.cpp
    mlock.h mlock.cpp
    syspage.h syspage.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE ubench GTest::gtest_main benchmark::benchmark)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

include_directories(${CMAKE_SOURCE_DIR}/benchmarks/malloc)

install(TARGETS ${BINARY} DESTINATION bin)

# Search for `mallopt()` features that can be used to customise the test.
check_symbol_exists(mallopt "malloc.h" HAVE_MALLOPT)
if(HAVE_MALLOPT)
    target_sources(${BINARY} PRIVATE mallopt_features.cpp)
else()
    target_sources(${BINARY} PRIVATE mallopt_none.cpp)
endif()

if (HAVE_MALLOPT)
    # Linux
    check_mallopt_option_exists(M_ARENA_MAX                  HAVE_M_ARENA_MAX)
    check_mallopt_option_exists(M_ARENA_TEST                 HAVE_M_ARENA_TEST)
    check_mallopt_option_exists(M_MMAP_MAX                   HAVE_M_MMAP_MAX)
    check_mallopt_option_exists(M_MMAP_THRESHOLD             HAVE_M_MMAP_THRESHOLD)
    check_mallopt_option_exists(M_MXFAST                     HAVE_M_MXFAST)
    check_mallopt_option_exists(M_PERTURB                    HAVE_M_PERTURB)
    check_mallopt_option_exists(M_TOP_PAD                    HAVE_M_TOP_PAD)
    check_mallopt_option_exists(M_TRIM_THRESHOLD             HAVE_M_TRIM_THRESHOLD)

    # QNX 7.1
    check_mallopt_option_exists(MALLOC_ARENA_CACHE_FREE_NOW  HAVE_MALLOC_ARENA_CACHE_FREE_NOW)
    check_mallopt_option_exists(MALLOC_ARENA_CACHE_MAXBLK    HAVE_MALLOC_ARENA_CACHE_MAXBLK)
    check_mallopt_option_exists(MALLOC_ARENA_CACHE_MAXSZ     HAVE_MALLOC_ARENA_CACHE_MAXSZ)
    check_mallopt_option_exists(MALLOC_ARENA_SIZE            HAVE_MALLOC_ARENA_SIZE)
    check_mallopt_option_exists(MALLOC_FREE_CHECK            HAVE_MALLOC_FREE_CHECK)
    check_mallopt_option_exists(MALLOC_MAX_ALIGNMENT         HAVE_MALLOC_MAX_ALIGNMENT)
    check_mallopt_option_exists(MALLOC_MEMORY_HOLD           HAVE_MALLOC_MEMORY_HOLD)
    check_mallopt_option_exists(MALLOC_VERIFY                HAVE_MALLOC_VERIFY)
    check_mallopt_option_exists(MALLOC_VERIFY_ON             HAVE_MALLOC_VERIFY_ON)

    # QNX 8.0
    check_mallopt_option_exists(M_GRANULARITY                HAVE_M_GRANULARITY)
endif()

check_symbol_exists(mlockall "sys/mman.h" HAVE_MLOCKALL)

target_clangformat_setup(${BINARY})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

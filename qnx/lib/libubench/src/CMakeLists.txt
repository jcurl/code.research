include(CheckSymbolExists)
include(CMakePushCheckState)

set(LIBRARY ubench)
set(SOURCES
    ../include/ubench/thread.h
)
add_library(${LIBRARY} STATIC ${SOURCES})

find_package(Threads REQUIRED)

cmake_push_check_state(RESET)
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
list(APPEND CMAKE_REQUIRED_LIBRARIES pthread)
check_symbol_exists(pthread_setaffinity_np pthread.h HAVE_PTHREAD_SETAFFINITY_NP)
cmake_reset_check_state()

# We need to be able to set the affinity of a thread to a particular CPU
if(HAVE_PTHREAD_SETAFFINITY_NP)
    add_compile_definitions(_GNU_SOURCE)
    target_sources(${LIBRARY} PRIVATE thread_pin.cpp)
else()
    # QNX doesn't have `pthread_getaffinity_np`, so we need to use `ThreadCtl`.
    check_symbol_exists(ThreadCtl "sys/neutrino.h" HAVE_NTO_THREADCTL)
    if(HAVE_NTO_THREADCTL)
        target_sources(${LIBRARY} PRIVATE thread_pin_qnx.cpp)
    else()
        target_sources(${LIBRARY} PRIVATE thread_pin_null.cpp)
        message(WARNING "Can't a way to pin threads to a core")
    endif()
endif()

target_compile_features(${LIBRARY} PRIVATE cxx_std_17)

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()

target_clangformat_setup(${LIBRARY})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

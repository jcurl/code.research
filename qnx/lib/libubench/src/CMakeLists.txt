include(CheckSymbolExists)
include(CMakePushCheckState)

set(LIBRARY ubench)
set(SOURCES
    ../include/ubench/thread.h
    ../include/ubench/clock.h
)
add_library(${LIBRARY} STATIC ${SOURCES})

find_package(Threads REQUIRED)

cmake_push_check_state(RESET)
list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
list(APPEND CMAKE_REQUIRED_LIBRARIES pthread)
check_symbol_exists(pthread_setaffinity_np pthread.h HAVE_PTHREAD_SETAFFINITY_NP)
cmake_reset_check_state()

if (QNXNTO)
    target_sources(${LIBRARY} PRIVATE thread_pin_qnx.cpp)
    target_sources(${LIBRARY} PRIVATE idle_clock_qnx.cpp)
    target_sources(${LIBRARY} PRIVATE process_clock_qnx.cpp)
else()
    # We need to be able to set the affinity of a thread to a particular CPU
    if(HAVE_PTHREAD_SETAFFINITY_NP)
        add_compile_definitions(_GNU_SOURCE)
        target_sources(${LIBRARY} PRIVATE thread_pin.cpp)
    else()
        target_sources(${LIBRARY} PRIVATE thread_pin_null.cpp)
        message(WARNING "Can't a way to pin threads to a core")
    endif()

    # TODO: If the tests fail, we need to use "null"
    string(TOUPPER "${CMAKE_SYSTEM_NAME}" upper_CMAKE_SYSTEM_NAME)
    if(upper_CMAKE_SYSTEM_NAME STREQUAL "LINUX")
        target_sources(${LIBRARY} PRIVATE idle_clock_linux.cpp)
        target_sources(${LIBRARY} PRIVATE process_clock_linux.cpp)
    else()
        target_sources(${LIBRARY} PRIVATE idle_clock_null.cpp)
        target_sources(${LIBRARY} PRIVATE process_clock_null.cpp)
    endif()
endif()

# We need to provide clocks based on the Operating System
target_compile_features(${LIBRARY} PRIVATE cxx_std_17)

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()

target_clangformat_setup(${LIBRARY})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

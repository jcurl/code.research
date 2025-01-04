include(CheckSymbolExists)
include(CMakePushCheckState)

set(LIBRARY ubench)
set(SOURCES
    ../include/ubench/args.h args.cpp
    ../include/ubench/clock.h
    ../include/ubench/file.h
    sync_event.cpp
    ../include/ubench/os.h
    ../include/ubench/thread.h
)
add_library(${LIBRARY} STATIC ${SOURCES})

find_package(Threads REQUIRED)

# BSD Variable only available in CMake 3.25 or later.
# LINUX Variable not available in CMake 3.16 (Ubuntu 20.04)
string(TOUPPER "${CMAKE_SYSTEM_NAME}" upper_CMAKE_SYSTEM_NAME)

# Clocks
if (QNXNTO)
    target_sources(${LIBRARY} PRIVATE idle_clock_qnx.cpp     process_clock_qnx.cpp)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")
    target_sources(${LIBRARY} PRIVATE idle_clock_cygwin.cpp  process_clock_cygwin.cpp)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "LINUX")
    target_sources(${LIBRARY} PRIVATE idle_clock_linux.cpp   process_clock_linux.cpp)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "NETBSD")
    target_sources(${LIBRARY} PRIVATE idle_clock_null.cpp    process_clock_linux.cpp)
else()
    target_sources(${LIBRARY} PRIVATE idle_clock_null.cpp    process_clock_null.cpp)
endif()

if (QNXNTO)
    target_sources(${LIBRARY} PRIVATE thread_pin_qnx.cpp)
    target_sources(${LIBRARY} PRIVATE get_proc_name_qnx.cpp get_proc_name_common.cpp get_proc_name_common.h)
else()
    # We need to be able to set the affinity of a thread to a particular CPU
    cmake_push_check_state(RESET)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
    list(APPEND CMAKE_REQUIRED_LIBRARIES pthread)
    check_symbol_exists(pthread_setaffinity_np pthread.h HAVE_PTHREAD_SETAFFINITY_NP)
    cmake_reset_check_state()

    if(HAVE_PTHREAD_SETAFFINITY_NP)
        add_compile_definitions(_GNU_SOURCE)
        target_sources(${LIBRARY} PRIVATE thread_pin.cpp)
    else()
        target_sources(${LIBRARY} PRIVATE thread_pin_null.cpp)
        message(WARNING "Can't a way to pin threads to a core")
    endif()

    if(upper_CMAKE_SYSTEM_NAME STREQUAL "LINUX")
        target_sources(${LIBRARY} PRIVATE get_proc_name_linux.cpp get_proc_name_common.cpp get_proc_name_common.h)
    elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")
        target_sources(${LIBRARY} PRIVATE get_proc_name_linux.cpp get_proc_name_common.cpp get_proc_name_common.h)
    else()
        target_sources(${LIBRARY} PRIVATE get_proc_name_null.cpp)
    endif()
endif()

# We need to provide clocks based on the Operating System
target_compile_features(${LIBRARY} PRIVATE cxx_std_17)

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()

target_clangformat_setup(${LIBRARY})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

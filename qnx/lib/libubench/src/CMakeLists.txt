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

# BSD Variable only available in CMake 3.25 or later.
# LINUX Variable not available in CMake 3.16 (Ubuntu 20.04)
string(TOUPPER "${CMAKE_SYSTEM_NAME}" upper_CMAKE_SYSTEM_NAME)

# Clocks
if (QNXNTO)
    target_sources(${LIBRARY} PRIVATE idle_clock_qnx.cpp     process_clock_qnx.cpp)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")
    target_sources(${LIBRARY} PRIVATE idle_clock_cygwin.cpp  process_clock_cygwin.cpp)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "LINUX")
    target_sources(${LIBRARY} PRIVATE idle_clock_linux.cpp   process_clock_linux.cpp  base_clock.h)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "NETBSD" OR upper_CMAKE_SYSTEM_NAME STREQUAL "FREEBSD")
    target_sources(${LIBRARY} PRIVATE idle_clock_bsd.cpp     process_clock_linux.cpp  base_clock.h)
else()
    target_sources(${LIBRARY} PRIVATE idle_clock_null.cpp    process_clock_null.cpp)
endif()

# Process Name
if (QNXNTO)
    target_sources(${LIBRARY} PRIVATE get_proc_name_qnx.cpp   get_proc_name_common.cpp get_proc_name_common.h)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "CYGWIN" OR
       upper_CMAKE_SYSTEM_NAME STREQUAL "LINUX" OR
       upper_CMAKE_SYSTEM_NAME STREQUAL "NETBSD")
    target_sources(${LIBRARY} PRIVATE get_proc_name_linux.cpp get_proc_name_common.cpp get_proc_name_common.h)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "FREEBSD")
    target_sources(${LIBRARY} PRIVATE get_proc_name_freebsd.cpp)
    target_link_libraries(${LIBRARY} "util")
else()
    target_sources(${LIBRARY} PRIVATE get_proc_name_null.cpp)
endif()

# Thread Affinity
if (QNXNTO)
    target_sources(${LIBRARY} PRIVATE thread_pin_qnx.cpp)
else()
    # We need to be able to set the affinity of a thread to a particular CPU
    find_package(Threads REQUIRED)
    cmake_push_check_state(RESET)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
    list(APPEND CMAKE_REQUIRED_LIBRARIES pthread)
    check_symbol_exists(pthread_setaffinity_np "pthread.h" HAVE_PTHREAD_SETAFFINITY_NP)
    if (NOT HAVE_PTHREAD_SETAFFINITY_NP)
        # Test for FreeBSD.
        check_include_file("pthread_np.h" HAVE_INCLUDE_PTHREAD_NP_H)
        if (HAVE_INCLUDE_PTHREAD_NP_H)
            unset(HAVE_PTHREAD_SETAFFINITY_NP CACHE)
            check_symbol_exists(pthread_setaffinity_np "pthread_np.h" HAVE_PTHREAD_SETAFFINITY_NP)
        endif()
    endif()
    cmake_reset_check_state()

    if(HAVE_PTHREAD_SETAFFINITY_NP)
        add_compile_definitions(_GNU_SOURCE)
        target_sources(${LIBRARY} PRIVATE thread_pin.cpp)
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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

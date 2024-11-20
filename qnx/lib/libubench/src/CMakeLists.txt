include(CheckSymbolExists)
include(CMakePushCheckState)
include(research/sockets)

set(LIBRARY ubench)
set(SOURCES
    ../include/ubench/thread.h
    ../include/ubench/clock.h
    sync_event.cpp
    ../include/ubench/args.h args.cpp
    ../include/ubench/net.h net_common.cpp
)
add_library(${LIBRARY} STATIC ${SOURCES})

find_package(Threads REQUIRED)

if (QNXNTO)
    target_sources(${LIBRARY} PRIVATE thread_pin_qnx.cpp)
    target_sources(${LIBRARY} PRIVATE idle_clock_qnx.cpp)
    target_sources(${LIBRARY} PRIVATE process_clock_qnx.cpp)
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

    string(TOUPPER "${CMAKE_SYSTEM_NAME}" upper_CMAKE_SYSTEM_NAME)
    if(upper_CMAKE_SYSTEM_NAME STREQUAL "LINUX")
        target_sources(${LIBRARY} PRIVATE idle_clock_linux.cpp)
        target_sources(${LIBRARY} PRIVATE process_clock_linux.cpp)
    elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")
        target_sources(${LIBRARY} PRIVATE idle_clock_cygwin.cpp)
        target_sources(${LIBRARY} PRIVATE process_clock_cygwin.cpp)
    else()
        target_sources(${LIBRARY} PRIVATE idle_clock_null.cpp)
        target_sources(${LIBRARY} PRIVATE process_clock_null.cpp)
    endif()
endif()

# Determine features for networking
find_libsocket(PROJECT ${LIBRARY} FATAL)
check_symbol_exists(getifaddrs "sys/types.h;ifaddrs.h" HAVE_NET_GETIFADDRS)
check_include_file("linux/if.h" HAVE_NET_INCLUDE_LINUX_IF)
check_symbol_exists(IFF_UP "net/if.h" HAVE_NET_IFF_UP)
check_symbol_exists(IFF_LOOPBACK "net/if.h" HAVE_NET_IFF_LOOPBACK)
check_symbol_exists(IFF_POINTOPOINT "net/if.h" HAVE_NET_IFF_POINTOPOINT)
check_symbol_exists(IFF_MULTICAST "net/if.h" HAVE_NET_IFF_MULTICAST)
check_symbol_exists(IFF_BROADCAST "net/if.h" HAVE_NET_IFF_BROADCAST)

# TODO: Should put this in a function for multiple checks. Can we make this simpler?
# e.g.
#  check_symbol_exists_multiple(<symbol> <defaulthdr> <varsym> HEADER <hdr1> <varhdr1> HEADER <hdr2> <varhdr2>)
#
# - Check exactly like "check_symbol_exists(<symbol> <defaulthdr> <varsym>)"
# - If found, then we can just exit. It should already be cached.
# - Note if it wasn't found in a previous 'check_symbol_exists', then the check is also skipped.
# - Unset the cache variable, because we want to keep searching
# - Run "check_symbol_exists(<symbol <hdr1> <varsym>)"
# - If found, then also set <varhdr1> so we know where it is found
# - Repeat until no more headers
message(STATUS "Looking for IFF_LOWER_UP")
check_symbol_exists(IFF_LOWER_UP "net/if.h" HAVE_NET_IFF_LOWER_UP)
if (NOT HAVE_NET_IFF_LOWER_UP AND HAVE_NET_INCLUDE_LINUX_IF)
  unset(HAVE_NET_IFF_LOWER_UP CACHE)
  check_symbol_exists(IFF_LOWER_UP "linux/if.h" HAVE_NET_IFF_LOWER_UP)
endif()
if (HAVE_NET_IFF_LOWER_UP)
    message(STATUS "Looking for IFF_LOWER_UP - found")
else()
    message(STATUS "Looking for IFF_LOWER_UP - not found")
endif()
unset(CMAKE_REQUIRED_QUIET)

if(HAVE_NET_GETIFADDRS)
    target_sources(${LIBRARY} PRIVATE net_query_getifaddrs.cpp)
endif()



target_compile_features(${LIBRARY} PRIVATE cxx_std_17)

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()

target_clangformat_setup(${LIBRARY})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

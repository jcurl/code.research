include(CheckSymbolExists)
include(research/check_symbol_lib_exists)
include(research/check_symbol_enum_exists)
include(research/check_symbol_gnusource_exists)

find_package(Threads REQUIRED)

set(BINARY udp_load)
set(SOURCES
    udp_load.cpp
    udp_talker.cpp udp_talker_bsdipv4.cpp udp_talker.h
    options.cpp options.h
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE Threads::Threads)
target_link_libraries(${BINARY} PRIVATE ubench)

check_symbol_lib_exists(socket "sys/socket.h" HAVE_SOCKET LIB "socket" TARGET ${BINARY} FATAL)
check_symbol_enum_exists(SO_REUSEPORT "sys/socket.h" HAVE_SO_REUSEPORT)

# Test using `sendmmsg`.
check_symbol_gnusource_exists(sendmmsg "sys/socket.h" HAVE_SENDMMSG)

# Test using `/dev/bpf` on NetBSD, FreeBSD, QNX 7.1. QNX 8.0 also needs "sys/socket.h"
check_symbol_enum_exists(BIOCSETIF "sys/socket.h;sys/types.h;sys/time.h;sys/ioctl.h;net/bpf.h" HAVE_BIOCSETIF)
if (HAVE_BIOCSETIF)
    target_sources(${BINARY} PRIVATE udp_talker_bpf.cpp)
endif()

# Timeout testing, used on FreeBSD when ENOBUFS are returned for a delay smaller
# than the scheduling tick time (10ms).
check_symbol_enum_exists(CLOCK_REALTIME_PRECISE "time.h" HAVE_CLOCK_REALTIME_PRECISE)
check_symbol_enum_exists(CLOCK_REALTIME "time.h" HAVE_CLOCK_REALTIME)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat(TARGET ${BINARY} DIR .)
target_use_msg(${BINARY} udp_load.use DESCRIPTION "UDP Stress Test tool")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
install(TARGETS ${BINARY} DESTINATION bin)

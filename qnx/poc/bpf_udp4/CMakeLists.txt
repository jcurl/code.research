include(CheckSymbolExists)
include(research/check_symbol_lib_exists)
include(research/check_symbol_enum_exists)

find_package(Threads REQUIRED)

set(BINARY bpf_udp4)
set(SOURCES
    bpf_udp4.cpp
    options.h options.cpp
    udp_socket.h
    bsd_socket.h bsd_socket.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE ubench)

check_symbol_lib_exists(socket "sys/socket.h" HAVE_SOCKET LIB "socket" TARGET ${BINARY} FATAL)
check_symbol_enum_exists(SO_REUSEPORT "sys/socket.h" HAVE_SO_REUSEPORT)

# Test using `/dev/bpf` on NetBSD, FreeBSD, QNX 7.1. QNX 8.0 also needs "sys/socket.h"
check_symbol_enum_exists(BIOCSETIF "sys/socket.h;sys/types.h;sys/time.h;sys/ioctl.h;net/bpf.h" HAVE_BIOCSETIF)
if (HAVE_BIOCSETIF)
    target_sources(${BINARY} PRIVATE bpf_socket.h bpf_socket.cpp bpf_l2_udp_packet.h bpf_l2_udp_packet.cpp)
endif()

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat(TARGET ${BINARY} DIR .)
target_use_msg(${BINARY} bpf_udp4.use DESCRIPTION "POC Sending UDPv4 packetsl")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
install(TARGETS ${BINARY} DESTINATION bin)

include(CheckSymbolExists)
include(sockets)

set(BINARY udp_load)
set(SOURCES
    udp_load.cpp
    busy_measurement.cpp busy_measurement.h
    udp_talker.cpp udp_talker_bsdipv4.cpp udp_talker.h
    options.cpp options.h
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} ubench)

find_libsocket(PROJECT ${BINARY} FATAL)
check_symbol_exists(SO_REUSEPORT "sys/socket.h" HAVE_SO_REUSEPORT)
check_symbol_exists(sendmmsg "sys/socket.h" HAVE_SENDMMSG)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat_setup(${BINARY})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
install(TARGETS ${BINARY} DESTINATION bin)

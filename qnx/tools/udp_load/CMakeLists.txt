include(CheckSymbolExists)

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

# QNX and some systems must link with -lsocket
check_symbol_exists(bind "arpa/inet.h" HAVE_LIBSOCKET)
if(NOT HAVE_LIBSOCKET)
    check_library_exists("socket" "bind" "" HAVE_LIBSOCKET_LIB)
    if(HAVE_LIBSOCKET_LIB)
        target_link_libraries(${BINARY} socket)
    else()
        message(FATAL_ERROR "No socket libraries found")
    endif()
endif()

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat_setup(${BINARY})

# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

install(TARGETS ${BINARY} DESTINATION bin)

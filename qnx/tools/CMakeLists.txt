add_subdirectory(time_compare)

if (NOT APPLE)
    add_subdirectory(core_latency)
endif()

if (QNXNTO)
    add_subdirectory(lsqf)
endif()

add_subdirectory(udp_load)

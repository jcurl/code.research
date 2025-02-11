add_subdirectory(time_compare)
add_subdirectory(udp_load)
add_subdirectory(iplist)

if(NOT APPLE)
    add_subdirectory(core_latency)
endif()

if(QNXNTO)
    add_subdirectory(lsqf)
    add_subdirectory(lshmem)
endif()

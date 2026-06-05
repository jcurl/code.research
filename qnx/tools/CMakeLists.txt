add_subdirectory(time_compare)
add_subdirectory(udp_load)
add_subdirectory(iplist)
add_subdirectory(cpuid-x86)
add_subdirectory(sdd)

if(NOT APPLE)
    add_subdirectory(core_latency)
endif()

if(QNXNTO)
    add_subdirectory(lsqf)
    add_subdirectory(lshmem)
endif()

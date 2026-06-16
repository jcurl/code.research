add_subdirectory(bpf_udp4)

if(QNXNTO)
    add_subdirectory(qnx_statvfs)
endif()

target_clangformat(TARGET bpf_udp4 DIR bpf_udp4)
target_clangformat(TARGET qnx_statvfs DIR qnx_statvfs)

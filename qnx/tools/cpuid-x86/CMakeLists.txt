add_subdirectory(libcpuid)

if (QNXNTO AND HAVE_CPUIDX86)
    add_subdirectory(devc-cpuid)
endif()

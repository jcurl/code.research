if(ENABLE_TEST)
    enable_testing()
endif()

add_subdirectory(libubench)
add_subdirectory(libstdext)

if(QNXNTO)
    add_subdirectory(libosqnx)
endif()
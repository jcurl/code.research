add_subdirectory(libubench)
add_subdirectory(libunet)
add_subdirectory(libstdext)

if(QNXNTO)
    add_subdirectory(libosqnx)
endif()

target_clangformat(TARGET libubench DIR_RECURSE libubench/src DIR_RECURSE libubench/include DIR_RECURSE libubench/test DIR_RECURSE libubench/benchmark)
target_clangformat(TARGET libstdext DIR_RECURSE libstdext/test)
target_clangformat(TARGET libunet DIR_RECURSE libunet/src DIR_RECURSE libunet/include DIR_RECURSE libunet/test DIR_RECURSE libunet/benchmark)
target_clangformat(TARGET libosqnx DIR_RECURSE libosqnx/src DIR_RECURSE libosqnx/include DIR_RECURSE libosqnx/test)

add_subdirectory(malloc)
add_subdirectory(cacheline)
add_subdirectory(strcomp)

if (QNXNTO)
  add_subdirectory(devctl)
endif()
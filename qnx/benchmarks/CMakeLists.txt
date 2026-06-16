add_subdirectory(malloc)
add_subdirectory(cacheline)
add_subdirectory(strcomp)

if (QNXNTO)
  add_subdirectory(devctl)
endif()

target_clangformat(TARGET cacheline DIR cacheline)
target_clangformat(TARGET dev-adder DIR devctl)
target_clangformat(TARGET malloc_bench DIR malloc)
target_clangformat(TARGET strcmp_bench DIR strcomp)

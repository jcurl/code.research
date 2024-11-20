add_subdirectory(src)
add_subdirectory(benchmark)

if(ENABLE_TEST)
    add_subdirectory(test)
endif()

set(BINARY iplist)
set(SOURCES iplist.cpp)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE ubench)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat(TARGET ${BINARY} DIR .)
target_use_msg(${BINARY} iplist.use DESCRIPTION "show all IPv4 addresses")

install(TARGETS ${BINARY} DESTINATION bin)

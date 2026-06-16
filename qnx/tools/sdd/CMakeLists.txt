add_subdirectory(libsjson)

set(BINARY sdd)
set(SOURCES
    sdd.cpp
    options.h
    options.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE libubench)
target_link_libraries(${BINARY} PRIVATE libstdext)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_use_msg(${BINARY} sdd.use DESCRIPTION "Simple Discovery Daemon")

install(TARGETS ${BINARY} DESTINATION bin)

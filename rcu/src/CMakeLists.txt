set(BINARY ${CMAKE_PROJECT_NAME})

set(SOURCES
    rcu.h
    main.cpp
)

#add_library(${BINARY}-lib STATIC ${SOURCES})
add_executable(${BINARY} ${SOURCES})

#target_link_libraries(${BINARY} ${BINARY}-lib)
target_compile_features(${BINARY} PRIVATE cxx_std_17)

if(IS_DEBUG)
    add_sanitizers(rcutest)
endif()

target_clangformat_setup(${BINARY} rcu.h)

include_directories(${CMAKE_SOURCE_DIR}/src)

install(TARGETS ${BINARY} DESTINATION bin)

set(BINARY lsqf)
set(SOURCES
    lsqf.cpp
    pids.h pids.cpp
    file.h file.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} ubench)

if(CMAKE_SYSTEM_VERSION VERSION_LESS 8.0)
    # QNX 8.0 and later automatically include the std::filesystem extensions
    target_link_libraries(${BINARY} c++fs)
endif()

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat_setup(${BINARY})

install(TARGETS ${BINARY} DESTINATION bin)

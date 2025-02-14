set(BINARY lsqf)
set(SOURCES
    lsqf.cpp
    file.h file.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} ubench osqnx)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat_setup(${BINARY})
target_use_msg(${BINARY} lsqf.use DESCRIPTION "list open qnx files (like lsof on Linux)")

install(TARGETS ${BINARY} DESTINATION bin)

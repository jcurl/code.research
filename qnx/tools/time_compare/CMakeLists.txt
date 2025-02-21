set(BINARY time_compare)
set(SOURCES time_compare.cpp)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat(TARGET ${BINARY} DIR .)
target_use_msg(${BINARY} time_compare.use DESCRIPTION "Time source comparison tool")

install(TARGETS ${BINARY} DESTINATION bin)

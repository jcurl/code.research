include(CheckSymbolExists)
include(research/check_symbol_lib_exists)
include(research/check_symbol_enum_exists)

set(BINARY qnx_statvfs)
set(SOURCES
    qnx_statvfs.cpp
    options.h options.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE ubench)

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat(TARGET ${BINARY} DIR .)
target_use_msg(${BINARY} qnx_statvfs.use DESCRIPTION "POC for getting block statistics")

install(TARGETS ${BINARY} DESTINATION bin)

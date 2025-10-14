set(DEVADDER_BINARY dev-adder)
set(DEVADDER_SOURCES dev-adder.cpp dev-adder.h)

add_executable(${DEVADDER_BINARY} ${DEVADDER_SOURCES})
target_compile_features(${DEVADDER_BINARY} PRIVATE cxx_std_17)
target_link_libraries(${DEVADDER_BINARY} PRIVATE ubench osqnx)
target_use_msg(${DEVADDER_BINARY} dev-adder.use DESCRIPTION "Simple software driver to add the value of one")

set(ADDER_BINARY adder)
set(ADDER_SOURCES adder.cpp)

add_executable(${ADDER_BINARY} ${ADDER_SOURCES})
target_compile_features(${ADDER_BINARY} PRIVATE cxx_std_17)
target_link_libraries(${ADDER_BINARY} PRIVATE ubench osqnx)
target_use_msg(${ADDER_BINARY} adder.use DESCRIPTION "Uses dev-adder to test speed of devctl")

if(IS_DEBUG)
    add_sanitizers(${DEVADDER_BINARY})
endif()

target_clangformat(TARGET ${ADDER_BINARY} DIR .)

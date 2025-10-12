set(DEVMGR_BINARY dev-resmgr)
set(DEVMGR_SOURCES
    device_mgr.cpp
)

add_executable(${DEVMGR_BINARY} ${DEVMGR_SOURCES})
target_compile_features(${DEVMGR_BINARY} PRIVATE cxx_std_17)
target_link_libraries(${DEVMGR_BINARY} PRIVATE osqnx)

SET(RESTEST_BINARY resmgr_device_test)
SET(RESTEST_SOURCES
    device_test.cpp
)

add_executable(${RESTEST_BINARY} ${RESTEST_SOURCES})
target_compile_features(${RESTEST_BINARY} PRIVATE cxx_std_17)
target_link_libraries(${RESTEST_BINARY} PRIVATE GTest::gtest_main gmock_main osqnx)
add_test(NAME ${RESTEST_BINARY} COMMAND ${RESTEST_BINARY})

if(IS_DEBUG)
    add_sanitizers(${DEVMGR_BINARY})
endif()

target_clangformat(TARGET rsrc_mgr DIR .)

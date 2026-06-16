include(GoogleTest)

set(BINARY cpuid_test)
set(SOURCES
    cpuid_test.cpp
    cpuid_cache_test.cpp
    cpuid_cpuctl_test.cpp
    cpuid_dev_test.cpp
    cpuid_native_test.cpp
    cpuidreader_cache_test.cpp
    cpuidreader_cpuctl_test.cpp
    cpuidreader_dev_test.cpp
    cpuidreader_native_test.cpp
    cpuidreader_xml_test.cpp
)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)
target_link_libraries(${BINARY} PRIVATE GTest::gtest_main gmock_main libcpuid libubench)
add_test(NAME ${BINARY} COMMAND ${BINARY})

find_package(LibXml2)
if (LibXml2_FOUND)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/i9-12900K.xml         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/Dell_c840.xml         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/invalid_empty.xml     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/invalid_root.xml      DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/invalid_noproc.xml    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/invalid_line1.xml     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/invalid_line2.xml     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/invalid_line3.xml     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/invalid_line4.xml     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/invalid_line5.xml     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/invalid_line6.xml     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/invalid_line7.xml     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/invalid_line8.xml     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/valid_line_space1.xml DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/valid_line_space2.xml DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/valid_line_space3.xml DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test_assets/valid_line_ff.xml     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test_assets)
endif()

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_code_coverage(${BINARY}
    EXCLUDE
    ${CMAKE_SOURCE_DIR}/test/*
    ${CMAKE_BINARY_DIR}/build/*
)

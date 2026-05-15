set(BINARY time_compare)
set(SOURCES time_compare.cpp)

add_executable(${BINARY} ${SOURCES})
target_compile_features(${BINARY} PRIVATE cxx_std_17)

# Check for QTIME flags.
if(QNXNTO)
    check_symbol_enum_exists(QTIME_FLAG_TIMER_ON_CPU0 "sys/syspage.h" HAVE_QTIME_FLAG_TIMER_ON_CPU0)
    check_symbol_enum_exists(QTIME_FLAG_CHECK_STABLE "sys/syspage.h" HAVE_QTIME_FLAG_CHECK_STABLE)
    check_symbol_enum_exists(QTIME_FLAG_TICKLESS "sys/syspage.h" HAVE_QTIME_FLAG_TICKLESS)
    check_symbol_enum_exists(QTIME_FLAG_TIMECC "sys/syspage.h" HAVE_QTIME_FLAG_TIMECC)
    check_symbol_enum_exists(QTIME_FLAG_GLOBAL_CLOCKCYCLES "sys/syspage.h" HAVE_QTIME_FLAG_GLOBAL_CLOCKCYCLES)
endif()

if(IS_DEBUG)
    add_sanitizers(${BINARY})
endif()

target_clangformat(TARGET ${BINARY} DIR .)
target_use_msg(${BINARY} time_compare.use DESCRIPTION "Time source comparison tool")

target_include_directories(${BINARY} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

install(TARGETS ${BINARY} DESTINATION bin)

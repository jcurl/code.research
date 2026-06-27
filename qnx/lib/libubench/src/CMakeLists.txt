include(CheckSymbolExists)
include(CMakePushCheckState)
include(CheckIncludeFiles)
include(research/check_type_exists)
include(research/check_symbol_enum_exists)
include(research/check_symbol_lib_exists)
include(research/check_symbol_gnusource_exists)
include(research/check_struct_field_exists)

set(LIBRARY libubench)
set(SOURCES
    ../include/ubench/clock.h
    ../include/ubench/flags.h
    ../include/ubench/file.h
    ../include/ubench/options.h options.cpp
    ../include/ubench/os.h
    ../include/ubench/string.h string.cpp
    ../include/ubench/str_intern.h str_intern.cpp
    ../include/ubench/thread.h
    ../include/ubench/measure/busy_measurement.h measure/busy_measurement.cpp
    ../include/ubench/measure/print.h measure/print.cpp
)
add_library(${LIBRARY} STATIC ${SOURCES})
set_target_properties(${LIBRARY} PROPERTIES OUTPUT_NAME "ubench")
target_include_directories(${LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_compile_features(${LIBRARY} PRIVATE cxx_std_17)
target_link_libraries(${LIBRARY} PUBLIC libstdext)

# BSD Variable only available in CMake 3.25 or later.
# LINUX Variable not available in CMake 3.16 (Ubuntu 20.04)
string(TOUPPER "${CMAKE_SYSTEM_NAME}" upper_CMAKE_SYSTEM_NAME)

# Clocks
if(QNXNTO)
    target_sources(${LIBRARY} PRIVATE idle_clock_qnx.cpp     process_clock_qnx.cpp)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")
    target_sources(${LIBRARY} PRIVATE idle_clock_cygwin.cpp  process_clock_cygwin.cpp)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "LINUX")
    target_sources(${LIBRARY} PRIVATE idle_clock_linux.cpp   process_clock_linux.cpp  base_clock.h)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "NETBSD" OR upper_CMAKE_SYSTEM_NAME STREQUAL "FREEBSD")
    target_sources(${LIBRARY} PRIVATE idle_clock_bsd.cpp     process_clock_linux.cpp  base_clock.h)
else()
    target_sources(${LIBRARY} PRIVATE idle_clock_null.cpp    process_clock_null.cpp)
endif()

# Process Name
if(QNXNTO)
    target_sources(${LIBRARY} PRIVATE get_proc_name_qnx.cpp   get_proc_name_common.cpp get_proc_name_common.h)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "CYGWIN" OR
       upper_CMAKE_SYSTEM_NAME STREQUAL "LINUX")
    target_sources(${LIBRARY} PRIVATE get_proc_name_linux.cpp get_proc_name_common.cpp get_proc_name_common.h)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "NETBSD")
    target_sources(${LIBRARY} PRIVATE get_proc_name_netbsd.cpp)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "FREEBSD")
    target_sources(${LIBRARY} PRIVATE get_proc_name_freebsd.cpp)
else()
    target_sources(${LIBRARY} PRIVATE get_proc_name_null.cpp)
endif()

# Syspage Size
if(upper_CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")
    target_sources(${LIBRARY} PRIVATE syspage_windows.cpp)
else()
    check_symbol_enum_exists(_SC_PAGESIZE "unistd.h" HAVE_SC_PAGESIZE)
    if(HAVE_SC_PAGESIZE)
        target_sources(${LIBRARY} PRIVATE syspage_sysconf.cpp)
    else()
        target_sources(${LIBRARY} PRIVATE syspage_none.cpp)
    endif()
endif()

# Executable Path
if(QNXNTO)
    target_sources(${LIBRARY} PRIVATE get_executable_path_qnx.cpp get_executable_path_common.cpp get_executable_path_common.h)
    if(CMAKE_SYSTEM_VERSION VERSION_LESS 8.0)
        # QNX 8.0 and later automatically include the std::filesystem extensions
        target_link_libraries(${LIBRARY} PRIVATE c++fs)
    endif()
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "CYGWIN" OR
       upper_CMAKE_SYSTEM_NAME STREQUAL "LINUX")
    target_sources(${LIBRARY} PRIVATE get_executable_path_linux.cpp get_executable_path_common.cpp get_executable_path_common.h)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "NETBSD" OR
       upper_CMAKE_SYSTEM_NAME STREQUAL "FREEBSD")
    target_sources(${LIBRARY} PRIVATE get_executable_path_bsd.cpp get_executable_path_common.cpp get_executable_path_common.h)
else()
    target_sources(${LIBRARY} PRIVATE get_executable_path_null.cpp)
endif()

# Thread Affinity
if(QNXNTO)
    target_sources(${LIBRARY} PRIVATE thread_pin_qnx.cpp)
else()
    # We need to be able to set the affinity of a thread to a particular CPU
    find_package(Threads REQUIRED)
    cmake_push_check_state()
    list(APPEND CMAKE_REQUIRED_LIBRARIES pthread)
    check_symbol_gnusource_exists(pthread_setaffinity_np "pthread.h" HAVE_PTHREAD_SETAFFINITY_NP)
    if (NOT HAVE_PTHREAD_SETAFFINITY_NP)
        # Test for FreeBSD.
        check_include_file("pthread_np.h" HAVE_INCLUDE_PTHREAD_NP_H)
        if (HAVE_INCLUDE_PTHREAD_NP_H)
            unset(HAVE_PTHREAD_SETAFFINITY_NP CACHE)
            check_symbol_exists(pthread_setaffinity_np "pthread_np.h" HAVE_PTHREAD_SETAFFINITY_NP)
        endif()
    endif()
    cmake_pop_check_state()

    if(HAVE_PTHREAD_SETAFFINITY_NP)
        if(upper_CMAKE_SYSTEM_NAME STREQUAL "NETBSD")
            target_sources(${LIBRARY} PRIVATE thread_pin_netbsd.cpp)
        else()
            target_sources(${LIBRARY} PRIVATE thread_pin.cpp)
        endif()
    else()
        target_sources(${LIBRARY} PRIVATE thread_pin_null.cpp)
        message(WARNING "Can't find a way to pin threads to a core")
    endif()
endif()

# Thread Count (number of cores)
if(upper_CMAKE_SYSTEM_NAME STREQUAL "LINUX")
    target_sources(${LIBRARY} PRIVATE thread_count_linux.cpp)
elseif(upper_CMAKE_SYSTEM_NAME STREQUAL "CYGWIN")
    target_sources(${LIBRARY} PRIVATE thread_count_windows.cpp)
else()
    check_symbol_enum_exists(_SC_NPROCESSORS_CONF "unistd.h" HAVE_SC_NPROCESSORS_CONF)
    if(HAVE_SC_NPROCESSORS_CONF)
        target_sources(${LIBRARY} PRIVATE thread_count_sysconf.cpp)
    else()
        target_sources(${LIBRARY} PRIVATE thread_count.cpp)
    endif()
endif()

# Check for strlcpy.
check_symbol_lib_exists(strlcpy "string.h" HAVE_STRLCPY)
check_symbol_lib_exists(strlcpy "bsd/string.h" HAVE_STRLCPY LIB bsd TARGET ${LIBRARY})
if(HAVE_STRLCPY)
    if(HAVE_STRLCPY_LINKLIBRARY)
        set(HAVE_INCLUDE_BSD_STRING_H 1)
    endif()
else()
    target_sources(${LIBRARY} PRIVATE string_strlcpy.cpp)
endif()

# String Interning
check_type_exists("std::pmr::memory_resource" "memory_resource" HAVE_CXX_MEMORY_RESOURCE)
if(NOT HAVE_CXX_MEMORY_RESOURCE)
    check_type_exists("std::experimental::pmr::memory_resource" "experimental/memory_resource" HAVE_CXX_EXPERIMENTAL_MEMORY_RESOURCE)
    if(NOT HAVE_CXX_EXPERIMENTAL_MEMORY_RESOURCE)
        message(FATAL_ERROR "No PMR resources available")
    endif()
endif()

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()

target_include_directories(${LIBRARY} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

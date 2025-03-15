include(CheckSymbolExists)
include(CMakePushCheckState)
include(research/check_type_exists)
include(research/check_symbol_enum_exists)
include(research/check_symbol_lib_exists)
include(research/check_struct_field_exists)

set(LIBRARY ubench)
set(SOURCES
    ../include/ubench/clock.h
    ../include/ubench/flags.h
    ../include/ubench/file.h
    ../include/ubench/net.h net_common.cpp net_common.h net_query_ioctls.cpp
    ../include/ubench/os.h
    ../include/ubench/string.h string.cpp
    ../include/ubench/str_intern.h str_intern.cpp
    ../include/ubench/thread.h
    ../include/ubench/measure/busy_measurement.h measure/busy_measurement.cpp
    ../include/ubench/measure/print.h measure/print.cpp
    sync_event.cpp
)
add_library(${LIBRARY} STATIC ${SOURCES})

target_compile_features(${LIBRARY} PRIVATE cxx_std_17)

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

# Thread Affinity
if(QNXNTO)
    target_sources(${LIBRARY} PRIVATE thread_pin_qnx.cpp)
else()
    # We need to be able to set the affinity of a thread to a particular CPU
    find_package(Threads REQUIRED)
    cmake_push_check_state(RESET)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
    list(APPEND CMAKE_REQUIRED_LIBRARIES pthread)
    check_symbol_exists(pthread_setaffinity_np "pthread.h" HAVE_PTHREAD_SETAFFINITY_NP)
    if (NOT HAVE_PTHREAD_SETAFFINITY_NP)
        # Test for FreeBSD.
        check_include_file("pthread_np.h" HAVE_INCLUDE_PTHREAD_NP_H)
        if (HAVE_INCLUDE_PTHREAD_NP_H)
            unset(HAVE_PTHREAD_SETAFFINITY_NP CACHE)
            check_symbol_exists(pthread_setaffinity_np "pthread_np.h" HAVE_PTHREAD_SETAFFINITY_NP)
        endif()
    endif()
    cmake_reset_check_state()

    if(HAVE_PTHREAD_SETAFFINITY_NP)
        add_compile_definitions(_GNU_SOURCE)
        target_sources(${LIBRARY} PRIVATE thread_pin.cpp)
    else()
        target_sources(${LIBRARY} PRIVATE thread_pin_null.cpp)
        message(WARNING "Can't a way to pin threads to a core")
    endif()
endif()

# ----------------------------------------------------------------------
# Determine features for NETWORKING

check_symbol_lib_exists(socket "sys/socket.h" HAVE_SOCKET LIB "socket" TARGET ${LIBRARY} FATAL)

check_symbol_exists(getifaddrs "sys/types.h;sys/socket.h;ifaddrs.h" HAVE_NET_GETIFADDRS)
if(HAVE_NET_GETIFADDRS)
    target_sources(${LIBRARY} PRIVATE net_query_getifaddrs.cpp)
endif()

check_symbol_enum_exists(SIOCGIFADDR "sys/ioctl.h;net/if.h" HAVE_NET_SIOCGIFADDR)
check_symbol_enum_exists(SIOCGIFCONF "sys/ioctl.h;net/if.h" HAVE_NET_SIOCGIFCONF)
check_symbol_enum_exists(SIOCGSIZIFCONF "sys/ioctl.h;net/if.h" HAVE_NET_SIOCGSIZIFCONF)
check_symbol_enum_exists(SIOCGIFBRDADDR "sys/ioctl.h;net/if.h" HAVE_NET_SIOCGIFBRDADDR)
check_symbol_enum_exists(SIOCGIFDSTADDR "sys/ioctl.h;net/if.h" HAVE_NET_SIOCGIFDSTADDR)
check_symbol_enum_exists(SIOCGIFFLAGS "sys/ioctl.h;net/if.h" HAVE_NET_SIOCGIFFLAGS)
check_symbol_enum_exists(SIOCGIFNETMASK "sys/ioctl.h;net/if.h" HAVE_NET_SIOCGIFNETMASK)

check_symbol_exists(SIOCGIFFRNDLYNAM "sys/ioctl.h;net/if.h" HAVE_NET_SIOCGIFFRNDLYNAM)
if(HAVE_NET_SIOCGIFFRNDLYNAM)
    target_sources(${LIBRARY} PRIVATE net_query_getalias.cpp)
endif()

# Cygwin has this in <net/if.h>. Some Linux distributions (Alpine) have this in
# <net/if.h>. Others (Ubuntu) have this in <linux/if.h>.
check_symbol_enum_exists(IFF_LOWER_UP "net/if.h" HAVE_NET_IFF_LOWER_UP MSG "in net/if.h" FORCE)
if(NOT HAVE_NET_IFF_LOWER_UP)
    check_symbol_enum_exists(IFF_LOWER_UP "linux/if.h" HAVE_NET_IFF_LOWER_UP MSG "in linux/if.h" FORCE)
    if(HAVE_NET_IFF_LOWER_UP)
        set(HAVE_NET_INCLUDE_LINUX_IF_H 1)
    endif()
endif()

# Check for VLAN support
check_symbol_enum_exists(SIOCGIFVLAN "sys/ioctl.h;net/if.h" HAVE_NET_SIOCGIFVLAN MSG "in net/if.h" FORCE)
if(NOT HAVE_NET_SIOCGIFVLAN)
    check_symbol_enum_exists(SIOCGIFVLAN "sys/ioctl.h;linux/sockios.h;net/if.h" HAVE_NET_SIOCGIFVLAN MSG "in linux/sockios.h" FORCE)
    if(HAVE_NET_SIOCGIFVLAN)
        set(HAVE_NET_INCLUDE_LINUX_SOCKIOS_H 1)
        target_sources(${LIBRARY} PRIVATE net_query_vlan_linux.cpp)
    endif()
endif()
if(NOT HAVE_NET_SIOCGIFVLAN)
    check_symbol_enum_exists(SIOCGETVLAN "sys/types.h;net/if_ether.h;net/if_vlanvar.h" HAVE_NET_SIOCGETVLAN MSG "in net/if_vlanvar.h" FORCE)
    if(HAVE_NET_SIOCGETVLAN)
        set(HAVE_NET_INCLUDE_NET_IF_VLANVAR_H 1)
        target_sources(${LIBRARY} PRIVATE net_query_vlan_bsd.cpp)
    endif()
    if(NOT HAVE_NET_SIOCGETVLAN)
        check_symbol_enum_exists(SIOCGETVLAN "sys/ioctl.h;sys/types.h;net/if.h;net/if_vlan_var.h" HAVE_NET_SIOCGETVLAN MSG "in net/if_vlan_var.h" FORCE)
        if(HAVE_NET_SIOCGETVLAN)
            set(HAVE_NET_INCLUDE_NET_IF_VLAN_VAR_H 1)
            target_sources(${LIBRARY} PRIVATE net_query_vlan_bsd.cpp)
        endif()
    endif()
endif()

# Check for AF_LINK and then use it for the HW MAC Address.
check_symbol_exists(AF_LINK "sys/socket.h" HAVE_NET_AF_LINK)
if(HAVE_NET_AF_LINK)
    check_type_exists("struct sockaddr_dl" "sys/types.h;sys/socket.h;net/if_dl.h" HAVE_NET_SOCKADDR_DL)
    if(HAVE_NET_SOCKADDR_DL)
        # Under QNX 7.1, NetBSD this is a `#define`. Under QNX 8, this is an ENUM.
        check_symbol_enum_exists(IFT_ETHER "sys/types.h;sys/socket.h;net/if_types.h" HAVE_NET_IFT_ETHER)
    endif()
endif()

# Check for HW MAC Address.
check_symbol_exists(SIOCGIFHWADDR "sys/ioctl.h;net/if.h" HAVE_NET_SIOCGIFHWADDR)
if(HAVE_NET_SIOCGIFHWADDR)
    check_include_file("net/ethernet.h" HAVE_NET_INCLUDE_NET_ETHERNET_H)
    target_sources(${LIBRARY} PRIVATE net_query_gethwaddr.cpp)
endif()

# Check for FreeBSD that flags is over two fields.
check_struct_field_exists("ifreq" "ifr_flagshigh" "net/if.h" NET_IFR_FLAGS_HIGH)

# ----------------------------------------------------------------------

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

target_clangformat(TARGET ${LIBRARY} DIR_RECURSE . DIR_RECURSE ../include/ubench)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include(CheckSymbolExists)
include(CMakePushCheckState)
include(CheckIncludeFiles)
include(research/check_type_exists)
include(research/check_symbol_enum_exists)
include(research/check_symbol_lib_exists)
include(research/check_symbol_gnusource_exists)
include(research/check_struct_field_exists)

set(LIBRARY libunet)
set(SOURCES
    ../include/ubench/net.h
    net_common.h net_common.cpp
    net_query_ioctls.cpp
    udp.cpp
)
add_library(${LIBRARY} STATIC ${SOURCES})
set_target_properties(${LIBRARY} PROPERTIES OUTPUT_NAME "unet")
target_include_directories(${LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_compile_features(${LIBRARY} PRIVATE cxx_std_17)
target_link_libraries(${LIBRARY} PUBLIC libstdext libubench)

# BSD Variable only available in CMake 3.25 or later.
# LINUX Variable not available in CMake 3.16 (Ubuntu 20.04)
string(TOUPPER "${CMAKE_SYSTEM_NAME}" upper_CMAKE_SYSTEM_NAME)

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

check_symbol_enum_exists(SO_REUSEADDR "sys/socket.h" HAVE_SO_REUSEADDR)
check_symbol_enum_exists(SO_REUSEPORT "sys/socket.h" HAVE_SO_REUSEPORT)

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
check_symbol_enum_exists(SIOCGIFVLAN "sys/ioctl.h;linux/sockios.h;net/if.h" HAVE_NET_SIOCGIFVLAN MSG "in linux/sockios.h" FORCE)
if(HAVE_NET_SIOCGIFVLAN)
    set(HAVE_NET_INCLUDE_LINUX_SOCKIOS_H 1)
    target_sources(${LIBRARY} PRIVATE net_query_vlan_linux.cpp)
else()
    check_symbol_enum_exists(SIOCGETVLAN "sys/types.h;net/if_ether.h;net/if_vlanvar.h" HAVE_NET_SIOCGETVLAN MSG "in net/if_vlanvar.h" FORCE)
    if(HAVE_NET_SIOCGETVLAN)
        set(HAVE_NET_INCLUDE_NET_IF_VLANVAR_H 1)
        target_sources(${LIBRARY} PRIVATE net_query_vlan_bsd.cpp)
    else()
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

# Check for getdomainname functions.
check_symbol_lib_exists(getnameinfo "sys/socket.h;netdb.h" HAVE_NETDB_GETNAMEINFO)
check_symbol_lib_exists(gethostent "sys/socket.h;netdb.h" HAVE_NETDB_GETHOSTENT)

if (HAVE_NETDB_GETNAMEINFO)
    # Cygwin, Linux, QNX, FreeBSD, NetBSD
    target_sources(${LIBRARY} PRIVATE net_domainname_nameinfo.cpp)
elseif(HAVE_NETDB_GETHOSTENT)
    # Linux, QNX, FreeBSD, NetBSD
    target_sources(${LIBRARY} PRIVATE net_domainname_hostent.cpp)
endif()

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()

target_include_directories(${LIBRARY} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

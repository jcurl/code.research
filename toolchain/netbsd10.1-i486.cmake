# To use this toolchain file
#
# cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/netbsd10.1-i486.cmake
#

# Intended to work with the Docker Containers build in the qnx/scripts
# folder. Update the `CMAKE_SYSROOT` and `tools` to match your system.

set(CMAKE_SYSTEM_NAME NetBSD)
set(CMAKE_SYSTEM_VERSION 10.1)
set(CMAKE_SYSTEM_PROCESSOR i486)

set(CMAKE_SYSROOT /opt/netbsd/10.1/i386/sysroot)
set(tools /opt/netbsd/10.1/i386/tools)

set(CMAKE_C_COMPILER ${tools}/bin/i486--netbsdelf-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/i486--netbsdelf-g++)

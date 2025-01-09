# To use this toolchain file
#
# cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/netbsd10.1-aarch64.cmake
#

# Intended to work with the Docker Containers build in the qnx/scripts
# folder. Update the `CMAKE_SYSROOT` and `tools` to match your system.

set(CMAKE_SYSTEM_NAME NetBSD)
set(CMAKE_SYSTEM_VERSION 10.1)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_SYSROOT /opt/netbsd/10.1/aarch64/sysroot)
set(tools /opt/netbsd/10.1/aarch64/tools)

set(CMAKE_C_COMPILER ${tools}/bin/aarch64--netbsd-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/aarch64--netbsd-g++)

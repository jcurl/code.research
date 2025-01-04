# To use this toolchain file
#
# cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/netbsd10.1-aarch64.cmake
#

set(CMAKE_SYSTEM_NAME NetBSD)
set(CMAKE_SYSTEM_VERSION 10.1)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_SYSROOT $ENV{HOME}/netbsd/10.1/aarch64/destdir)
set(tools $ENV{HOME}/netbsd/10.1/aarch64/tools)

set(CMAKE_C_COMPILER ${tools}/bin/aarch64--netbsd-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/aarch64--netbsd-g++)

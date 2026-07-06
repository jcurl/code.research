# To use this toolchain file
#
# cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/freebsd-15.1-aarch64.cmake
#

set(CMAKE_SYSTEM_NAME FreeBSD)
set(CMAKE_SYSTEM_VERSION 15.1)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_SYSROOT /opt/freebsd/15.1/aarch64/sysroot)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_FLAGS "-target aarch64-unknown-freebsd15")
set(CMAKE_CXX_FLAGS "-target aarch64-unknown-freebsd15")
set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -stdlib=libc++")
include_directories(BEFORE SYSTEM "/opt/freebsd/15.1/aarch64/sysroot/usr/include/c++/v1")

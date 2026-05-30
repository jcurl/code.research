# To use this toolchain file
#
# cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/netbsd10.1-aarch64-clang.cmake
#

# Intended to work with the Docker Containers build in the qnx/scripts
# folder. Update the `CMAKE_SYSROOT` and `tools` to match your system.

set(CMAKE_SYSTEM_NAME NetBSD)
set(CMAKE_SYSTEM_VERSION 10.1)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(sysroot /opt/netbsd/10.1/aarch64/sysroot)
set(tools /opt/netbsd/10.1/aarch64/tools)
set(CMAKE_SYSROOT ${sysroot})

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_FLAGS "-Qunused-arguments -target aarch64--netbsd10.1 --gcc-toolchain=${tools}")
set(CMAKE_CXX_FLAGS "-Qunused-arguments -target aarch64--netbsd10.1 --gcc-toolchain=${tools} --stdlib=libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -L${sysroot}/usr/lib --rtlib=libgcc --unwindlib=libgcc --stdlib=libstdc++ -lgcc_s")

# To use this toolchain file
#
# cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/qcc_x86_64.cmake
#

set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(arch gcc_ntox86_64)

set(CMAKE_C_COMPILER qcc)
set(CMAKE_C_COMPILER_TARGET ${arch})
set(CMAKE_CXX_COMPILER q++)
set(CMAKE_CXX_COMPILER_TARGET ${arch})

set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

set(LIBXML2_LIBRARY "$ENV{QNX_TARGET}/x86_64/usr/lib/libxml2.so")
set(LIBXML2_INCLUDE_DIR "$ENV{QNX_TARGET}/usr/include")

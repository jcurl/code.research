# To use this toolchain file
#
# cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/qcc_aarch64le-rpi5.cmake
#

set(CMAKE_SYSTEM_NAME QNX)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(arch gcc_ntoaarch64le)

set(CMAKE_C_COMPILER qcc)
set(CMAKE_C_COMPILER_TARGET ${arch})
set(CMAKE_CXX_COMPILER q++)
set(CMAKE_CXX_COMPILER_TARGET ${arch})

set(CMAKE_C_FLAGS_INIT "-march=armv8.1-a+lse")
set(CMAKE_CXX_FLAGS_INIT "-march=armv8.1-a+lse")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# If the imported libraries are treated as SYSTEM entries, and the path is `$ENV{QNX_TARGET}/usr/include`
# the CXX build will fail.
set(CMAKE_NO_SYSTEM_FROM_IMPORTED TRUE)
set(LIBXML2_LIBRARY "$ENV{QNX_TARGET}/aarch64le/usr/lib/libxml2.so" CACHE FILEPATH "")
set(LIBXML2_INCLUDE_DIR "$ENV{QNX_TARGET}/usr/include" CACHE PATH "")

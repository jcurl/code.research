# To use this toolchain file
#
# cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain/qcc_aarch64le.cmake
#

set(CMAKE_SYSTEM_PROCESSOR i386)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_COMPILER_TARGET i386)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_COMPILER_TARGET i386)

set(CMAKE_C_FLAGS "-m32")
set(CMAKE_CXX_FLAGS "-m32")
set(CMAKE_C_FLAGS_RELEASE "-m32 -O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-m32 -O2 -DNDEBUG")

set(LIBXML2_LIBRARY "/usr/lib/i386-linux-gnu/libxml2.so" CACHE FILEPATH "")
set(LIBXML2_INCLUDE_DIR "/usr/include/libxml2" CACHE PATH "")

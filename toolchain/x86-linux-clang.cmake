# Pure LLVM toolchain, based on https://clang.llvm.org/docs/Toolchain.html

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_LINKER lld)

# Silence -Wunused-command-line-argument warnings
# see: https://github.com/llvm/llvm-project/issues/64939
set(CMAKE_C_FLAGS "-Qunused-arguments -rtlib=compiler-rt")
set(CMAKE_CXX_FLAGS "-Qunused-arguments -rtlib=compiler-rt -stdlib=libc++")

set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=lld -lc++abi -lunwind")
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})

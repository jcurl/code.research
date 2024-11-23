all: alpine-latest-clang ubuntu-focal ubuntu-focal-clang ubuntu-jammy ubuntu-jammy-clang ubuntu-noble ubuntu-noble-clang

NUM_CPUS := $(shell nproc)

alpine-latest-clang:
	@./build.sh -dalpine -clatest -vclang "rm -rf * && cmake -B . -S /source/qnx -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/x86-linux-clang.cmake && cmake --build . -j${NUM_CPUS} && ctest"

ubuntu-focal:
	@./build.sh -cfocal "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && cmake --build . -j${NUM_CPUS} && ctest"

ubuntu-focal-clang:
	@./build.sh -cfocal -vclang "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/x86-linux-clang.cmake && cmake --build . -j${NUM_CPUS} && ctest"

ubuntu-jammy:
	@./build.sh -cjammy "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && cmake --build . -j${NUM_CPUS} && ctest"

ubuntu-jammy-clang:
	@./build.sh -cjammy -vclang "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/x86-linux-clang.cmake && cmake --build . -j${NUM_CPUS} && ctest"

ubuntu-noble:
	@./build.sh -cnoble "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && cmake --build . -j${NUM_CPUS} && ctest"

ubuntu-noble-clang:
	@./build.sh -cnoble -vclang "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/x86-linux-clang.cmake && cmake --build . -j${NUM_CPUS} && ctest"


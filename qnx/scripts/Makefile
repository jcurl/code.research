.PHONY: linux
linux: alpine-latest-clang ubuntu-focal ubuntu-focal-clang ubuntu-jammy ubuntu-jammy-clang ubuntu-noble ubuntu-noble-clang

.PHONY: netbsd
netbsd: netbsd10.1-aarch64 netbsd10.1-aarch64eb

.PHONY: freebsd
freebsd: freebsd14.3-aarch64 freebsd14.2-aarch64 freebsd13.5-aarch64

.PHONY: qnx
qnx: qnx-710-aarch64 qnx-710-x64 qnx-800-aarch64 qnx-800-x64

.PHONY: all
all: linux netbsd freebsd

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

ubuntu-plucky:
	@./build.sh -cplucky "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && cmake --build . -j${NUM_CPUS} && ctest"

ubuntu-plucky-clang:
	@./build.sh -cplucky -vclang "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/x86-linux-clang.cmake && cmake --build . -j${NUM_CPUS} && ctest"

netbsd10.1-aarch64:
	@./build.sh -dnetbsd -c10.1 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd10.1-aarch64.cmake && cmake --build . -j${NUM_CPUS}"

netbsd10.1-aarch64eb:
	@./build.sh -dnetbsd -c10.1 -veb "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd10.1-aarch64eb.cmake && cmake --build . -j${NUM_CPUS}"

freebsd13.5-aarch64:
	@./build.sh -dfreebsd -c13.5 "rm -rf * & cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/freebsd13.5-aarch64.cmake && cmake --build . -j${NUM_CPUS}"

freebsd14.2-aarch64:
	@./build.sh -dfreebsd -c14.2 "rm -rf * & cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/freebsd14.2-aarch64.cmake && cmake --build . -j${NUM_CPUS}"

freebsd14.3-aarch64:
	@./build.sh -dfreebsd -c14.3 "rm -rf * & cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/freebsd14.3-aarch64.cmake && cmake --build . -j${NUM_CPUS}"

qnx-710-aarch64:
	@./build.sh -dqnx -c710 -varm64 "rm -rf * & cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/qcc_aarch64le.cmake && cmake --build . -j${NUM_CPUS}"

qnx-710-x64:
	@./build.sh -dqnx -c710 -vx64 "rm -rf * & cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/qcc_aarch64le.cmake && cmake --build . -j${NUM_CPUS}"

qnx-800-aarch64:
	@./build.sh -dqnx -c800 -varm64 "rm -rf * & cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/qcc_aarch64le.cmake && cmake --build . -j${NUM_CPUS}"

qnx-800-x64:
	@./build.sh -dqnx -c800 -vx64 "rm -rf * & cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/qcc_aarch64le.cmake && cmake --build . -j${NUM_CPUS}"

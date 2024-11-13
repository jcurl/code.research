all: ubuntu-focal ubuntu-focal-clang ubuntu-jammy ubuntu-jammy-clang ubuntu-noble ubuntu-noble-clang

NUM_CPUS := $(shell nproc)

ubuntu-focal:
	@./build.sh -cfocal "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && make -j${NUM_CPUS}"

ubuntu-focal-clang:
	@./build.sh -cfocal -vclang "rm -rf * && CXX="clang++" C="clang" cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && make -j${NUM_CPUS}"

ubuntu-jammy:
	@./build.sh -cjammy "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && make -j${NUM_CPUS}"

ubuntu-jammy-clang:
	@./build.sh -cjammy -vclang "rm -rf * && CXX="clang++" C="clang" cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && make -j${NUM_CPUS}"

ubuntu-noble:
	@./build.sh -cnoble "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && make -j${NUM_CPUS}"

ubuntu-noble-clang:
	@./build.sh -cnoble -vclang "rm -rf * && CXX="clang++" C="clang" cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && make -j${NUM_CPUS}"


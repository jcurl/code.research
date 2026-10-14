# Environment variables that can be given when building with "make"
#
# TARGET="xxx"
#  A space separated list of targets to build.
#  - If not provided, then default is to test all
#  - If provided, no tests are defaulted (as it would run tests that
#    haven't been built)
#
# TESTTARGET="xxx"
#  A space separated list of regular expressions for running test
#  targets.
#  - If starts with "-", pass as ctest option (e.g., "-V")
#  - Otherwise, combine as regex pattern separated by "|" (e.g.,
#    "test1 test2" becomes "test1|test2")

# Define space as a literal space for use in subst function
empty:=
space:= $(empty) $(empty)

# Conditionally set CMAKE_TARGET based on whether TARGET is defined.
# If TESTTARGET is defined, then these targets are added to TARGET,
# except if it starts with an option.
ifdef TARGET
  ifdef TESTTARGET
    ifeq ($(filter -%,$(firstword $(TESTTARGET))),)
	  uniq = $(if $1,$(firstword $1) \
        $(call uniq,$(filter-out $(firstword $1),$1)))
	  CMAKE_TARGET := --target $(call uniq,$(TARGET) $(TESTTARGET))
      CMAKE_CTEST := && ctest -R \"$(subst $(space),|,$(TESTTARGET))\"
    else
      CMAKE_TARGET := --target ${TARGET}
	  CMAKE_CTEST := && ctest \"$(TESTTARGET)\"
	endif
  else
    CMAKE_TARGET := --target ${TARGET}
    CMAKE_CTEST :=
  endif
else
  ifdef TESTTARGET
    ifeq ($(filter -%,$(firstword $(TESTTARGET))),)
	  CMAKE_TARGET := --target ${TESTTARGET}
	  CMAKE_CTEST := && ctest -R \"$(subst $(space),|,$(TESTTARGET))\"
    else
	  CMAKE_TARGET :=
      CMAKE_CTEST := && ctest \"$(TESTTARGET)\"
	endif
  else
    CMAKE_TARGET :=
    CMAKE_CTEST := && ctest
  endif
endif

# Conditionally set NUM_CPUS to how many concurrent builds, or use the
# default if not provided.
ifndef NUM_CPUS
  NUM_CPUS := $(shell nproc)
endif

$(info ------------------------------------------------------------------------)
$(info CMAKE_TARGET : $(CMAKE_TARGET))
$(info CMAKE_CTEST  : $(CMAKE_CTEST))
$(info NUM_CPUS     : $(NUM_CPUS))
$(info ------------------------------------------------------------------------)

.PHONY: linux
linux: alpine-latest-clang ubuntu-focal ubuntu-focal-clang ubuntu-jammy ubuntu-jammy-clang ubuntu-noble ubuntu-noble-clang ubuntu-resolute ubuntu-resolute-clang

.PHONY: netbsd
netbsd: netbsd11.0-aarch64 netbsd10.2-aarch64 netbsd10.1-aarch64-clang

.PHONY:netbsd-all
netbsd-all: netbsd11.0-aarch64 netbsd11.0-aarch64eb netbsd11.0-x64 netbsd11.0-x86 netbsd10.2-aarch64 netbsd10.2-aarch64eb netbsd10.2-x64 netbsd10.2-x86 netbsd10.1-aarch64 netbsd10.1-aarch64-clang netbsd10.1-aarch64eb netbsd10.1-x64 netbsd10.1-x86

.PHONY: freebsd
freebsd: freebsd15.1-aarch64 freebsd14.5-aarch64 freebsd13.5-aarch64

.PHONY: freebsd-all
freebsd-all: freebsd15.1-aarch64 freebsd15.0-aarch64 freebsd14.5-aarch64 freebsd14.4-aarch64 freebsd14.3-aarch64 freebsd14.2-aarch64 freebsd13.5-aarch64

.PHONY: qnx
qnx: qnx-710-aarch64 qnx-710-x64 qnx-800-aarch64-rpi5 qnx-800-aarch64 qnx-800-x64

.PHONY: clang
clang: alpine-latest-clang ubuntu-resolute-clang ubuntu-noble-clang ubuntu-jammy-clang freebsd15.0-aarch64 freebsd14.4-aarch64 netbsd10.1-aarch64-clang

.PHONY: all
all: linux netbsd-all freebsd-all qnx

# Defining clangformat in a podman container ensures consistent formatting across all platforms.
clangformat:
	@./build.sh -dubuntu -cjammy -S -Bclangformat "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && cmake --build . --target clangformat"

alpine-latest-clang:
	@./build.sh -dalpine -clatest -vclang "rm -rf * && cmake -B . -S /source/qnx -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/x86-linux-clang.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET} ${CMAKE_CTEST}"

ubuntu-focal:
	@./build.sh -cfocal "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_RUN_CLANG_TIDY=off && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET} ${CMAKE_CTEST}"

ubuntu-focal-clang:
	@./build.sh -cfocal -vclang "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_RUN_CLANG_TIDY=off -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/x86-linux-clang.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET} ${CMAKE_CTEST}"

ubuntu-jammy:
	@./build.sh -cjammy "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET} ${CMAKE_CTEST}"

ubuntu-jammy-clang:
	@./build.sh -cjammy -vclang "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/x86-linux-clang.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET} ${CMAKE_CTEST}"

ubuntu-noble:
	@./build.sh -cnoble "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET} ${CMAKE_CTEST}"

ubuntu-noble-clang:
	@./build.sh -cnoble -vclang "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/x86-linux-clang.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET} ${CMAKE_CTEST}"

ubuntu-resolute:
	@./build.sh -cresolute "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET} ${CMAKE_CTEST}"

ubuntu-resolute-clang:
	@./build.sh -cresolute -vclang "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/x86-linux-clang.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET} ${CMAKE_CTEST}"

netbsd10.1-aarch64:
	@./build.sh -dnetbsd -c10.1 -varm64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd10.1-aarch64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

netbsd10.1-aarch64-clang:
	@./build.sh -dnetbsd -c10.1 -varm64clang "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd10.1-aarch64-clang.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

netbsd10.1-aarch64eb:
	@./build.sh -dnetbsd -c10.1 -varm64eb "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd10.1-aarch64eb.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

netbsd10.1-x64:
	@./build.sh -dnetbsd -c10.1 -vx64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd10.1-x86_64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

netbsd10.1-x86:
	@./build.sh -dnetbsd -c10.1 -vx86 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd10.1-i486.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

netbsd10.2-aarch64:
	@./build.sh -dnetbsd -c10.2 -varm64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd10.2-aarch64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

netbsd10.2-aarch64eb:
	@./build.sh -dnetbsd -c10.2 -varm64eb "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd10.2-aarch64eb.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

netbsd10.2-x64:
	@./build.sh -dnetbsd -c10.2 -vx64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd10.2-x86_64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

netbsd10.2-x86:
	@./build.sh -dnetbsd -c10.2 -vx86 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd10.2-i486.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

netbsd11.0-aarch64:
	@./build.sh -dnetbsd -c11.0 -varm64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd11.0-aarch64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

netbsd11.0-aarch64eb:
	@./build.sh -dnetbsd -c11.0 -varm64eb "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd11.0-aarch64eb.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

netbsd11.0-x64:
	@./build.sh -dnetbsd -c11.0 -vx64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd11.0-x86_64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

netbsd11.0-x86:
	@./build.sh -dnetbsd -c11.0 -vx86 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/netbsd11.0-i486.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

freebsd13.5-aarch64:
	@./build.sh -dfreebsd -c13.5 -varm64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/freebsd13.5-aarch64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

freebsd14.2-aarch64:
	@./build.sh -dfreebsd -c14.2 -varm64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/freebsd14.2-aarch64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

freebsd14.3-aarch64:
	@./build.sh -dfreebsd -c14.3 -varm64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/freebsd14.3-aarch64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

freebsd14.4-aarch64:
	@./build.sh -dfreebsd -c14.4 -varm64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/freebsd14.4-aarch64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

freebsd14.5-aarch64:
	@./build.sh -dfreebsd -c14.5 -varm64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/freebsd14.5-aarch64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

freebsd15.0-aarch64:
	@./build.sh -dfreebsd -c15.0 -bUBUNTU_VERSION=resolute -varm64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/freebsd15.0-aarch64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

freebsd15.1-aarch64:
	@./build.sh -dfreebsd -c15.1 -bUBUNTU_VERSION=resolute -varm64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/freebsd15.1-aarch64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

qnx-710-aarch64:
	@./build.sh -dqnx -c710 -varm64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/qcc_aarch64le.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

qnx-710-x64:
	@./build.sh -dqnx -c710 -vx64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/qcc_x86_64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

qnx-800-aarch64:
	@./build.sh -dqnx -c800 -varm64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/qcc_aarch64le.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

qnx-800-aarch64-rpi5:
	@./build.sh -dqnx -c800 -varm64-rpi5 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/qcc_aarch64le-rpi5.cmake -DCMAKE_BUILD_TYPE=Release && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

qnx-800-x64:
	@./build.sh -dqnx -c800 -vx64 "rm -rf * && cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/qcc_x86_64.cmake && cmake --build . -j${NUM_CPUS} ${CMAKE_TARGET}"

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

# Shared build settings used by every build rule below.
RELEASE := -DCMAKE_BUILD_TYPE=Release
BUILD_TEST := -j${NUM_CPUS} ${CMAKE_TARGET} ${CMAKE_CTEST}
BUILD_ONLY := -j${NUM_CPUS} ${CMAKE_TARGET}

# Cmake toolchain file in /source/toolchain, given without the .cmake suffix.
toolchain = -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/$(1).cmake

# $(1) build.sh arguments,
# $(2) cmake configure arguments,
# $(3) cmake --build arguments.
#
# With INC set (e.g. INC=1), the build directory is kept and cmake is not
# reconfigured, so only the build (and test) steps run.
define run_build
@./build.sh $(1) "$(if $(INC),,rm -rf * && cmake -B . -S /source/qnx $(2) && )cmake --build . $(3)"
endef

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
	$(call run_build,-dalpine -clatest -vclang,-G Ninja $(RELEASE) $(call toolchain,x86-linux-clang),$(BUILD_TEST))

ubuntu-focal:
	$(call run_build,-cfocal,$(RELEASE) -DCMAKE_RUN_CLANG_TIDY=off,$(BUILD_TEST))

ubuntu-focal-clang:
	$(call run_build,-cfocal -vclang,$(RELEASE) -DCMAKE_RUN_CLANG_TIDY=off $(call toolchain,x86-linux-clang),$(BUILD_TEST))

ubuntu-jammy:
	$(call run_build,-cjammy,$(RELEASE),$(BUILD_TEST))

ubuntu-jammy-clang:
	$(call run_build,-cjammy -vclang,$(RELEASE) $(call toolchain,x86-linux-clang),$(BUILD_TEST))

ubuntu-noble:
	$(call run_build,-cnoble,$(RELEASE),$(BUILD_TEST))

ubuntu-noble-clang:
	$(call run_build,-cnoble -vclang,$(RELEASE) $(call toolchain,x86-linux-clang),$(BUILD_TEST))

ubuntu-resolute:
	$(call run_build,-cresolute,$(RELEASE),$(BUILD_TEST))

ubuntu-resolute-clang:
	$(call run_build,-cresolute -vclang,$(RELEASE) $(call toolchain,x86-linux-clang),$(BUILD_TEST))

netbsd10.1-aarch64:
	$(call run_build,-dnetbsd -c10.1 -varm64,$(RELEASE) $(call toolchain,netbsd10.1-aarch64),$(BUILD_ONLY))

netbsd10.1-aarch64-clang:
	$(call run_build,-dnetbsd -c10.1 -varm64clang,$(RELEASE) $(call toolchain,netbsd10.1-aarch64-clang),$(BUILD_ONLY))

netbsd10.1-aarch64eb:
	$(call run_build,-dnetbsd -c10.1 -varm64eb,$(RELEASE) $(call toolchain,netbsd10.1-aarch64eb),$(BUILD_ONLY))

netbsd10.1-x64:
	$(call run_build,-dnetbsd -c10.1 -vx64,$(RELEASE) $(call toolchain,netbsd10.1-x86_64),$(BUILD_ONLY))

netbsd10.1-x86:
	$(call run_build,-dnetbsd -c10.1 -vx86,$(RELEASE) $(call toolchain,netbsd10.1-i486),$(BUILD_ONLY))

netbsd10.2-aarch64:
	$(call run_build,-dnetbsd -c10.2 -varm64,$(RELEASE) $(call toolchain,netbsd10.2-aarch64),$(BUILD_ONLY))

netbsd10.2-aarch64eb:
	$(call run_build,-dnetbsd -c10.2 -varm64eb,$(RELEASE) $(call toolchain,netbsd10.2-aarch64eb),$(BUILD_ONLY))

netbsd10.2-x64:
	$(call run_build,-dnetbsd -c10.2 -vx64,$(RELEASE) $(call toolchain,netbsd10.2-x86_64),$(BUILD_ONLY))

netbsd10.2-x86:
	$(call run_build,-dnetbsd -c10.2 -vx86,$(RELEASE) $(call toolchain,netbsd10.2-i486),$(BUILD_ONLY))

netbsd11.0-aarch64:
	$(call run_build,-dnetbsd -c11.0 -varm64,$(RELEASE) $(call toolchain,netbsd11.0-aarch64),$(BUILD_ONLY))

netbsd11.0-aarch64eb:
	$(call run_build,-dnetbsd -c11.0 -varm64eb,$(RELEASE) $(call toolchain,netbsd11.0-aarch64eb),$(BUILD_ONLY))

netbsd11.0-x64:
	$(call run_build,-dnetbsd -c11.0 -vx64,$(RELEASE) $(call toolchain,netbsd11.0-x86_64),$(BUILD_ONLY))

netbsd11.0-x86:
	$(call run_build,-dnetbsd -c11.0 -vx86,$(RELEASE) $(call toolchain,netbsd11.0-i486),$(BUILD_ONLY))

freebsd13.5-aarch64:
	$(call run_build,-dfreebsd -c13.5 -varm64,$(RELEASE) $(call toolchain,freebsd13.5-aarch64),$(BUILD_ONLY))

freebsd14.2-aarch64:
	$(call run_build,-dfreebsd -c14.2 -varm64,$(RELEASE) $(call toolchain,freebsd14.2-aarch64),$(BUILD_ONLY))

freebsd14.3-aarch64:
	$(call run_build,-dfreebsd -c14.3 -varm64,$(RELEASE) $(call toolchain,freebsd14.3-aarch64),$(BUILD_ONLY))

freebsd14.4-aarch64:
	$(call run_build,-dfreebsd -c14.4 -varm64,$(RELEASE) $(call toolchain,freebsd14.4-aarch64),$(BUILD_ONLY))

freebsd14.5-aarch64:
	$(call run_build,-dfreebsd -c14.5 -varm64,$(RELEASE) $(call toolchain,freebsd14.5-aarch64),$(BUILD_ONLY))

freebsd15.0-aarch64:
	$(call run_build,-dfreebsd -c15.0 -bUBUNTU_VERSION=resolute -varm64,$(RELEASE) $(call toolchain,freebsd15.0-aarch64),$(BUILD_ONLY))

freebsd15.1-aarch64:
	$(call run_build,-dfreebsd -c15.1 -bUBUNTU_VERSION=resolute -varm64,$(RELEASE) $(call toolchain,freebsd15.1-aarch64),$(BUILD_ONLY))

qnx-710-aarch64:
	$(call run_build,-dqnx -c710 -varm64,$(RELEASE) $(call toolchain,qcc_aarch64le),$(BUILD_ONLY))

qnx-710-x64:
	$(call run_build,-dqnx -c710 -vx64,$(RELEASE) $(call toolchain,qcc_x86_64),$(BUILD_ONLY))

qnx-800-aarch64:
	$(call run_build,-dqnx -c800 -varm64,$(RELEASE) $(call toolchain,qcc_aarch64le),$(BUILD_ONLY))

qnx-800-aarch64-rpi5:
	$(call run_build,-dqnx -c800 -varm64-rpi5,$(call toolchain,qcc_aarch64le-rpi5) $(RELEASE),$(BUILD_ONLY))

qnx-800-x64:
	$(call run_build,-dqnx -c800 -vx64,$(RELEASE) $(call toolchain,qcc_x86_64),$(BUILD_ONLY))

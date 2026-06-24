# Scripts <!-- omit in toc -->

- [1. Podman Container - build.sh](#1-podman-container---buildsh)
  - [1.1. Configuring Podman the First Time](#11-configuring-podman-the-first-time)
  - [1.2. Using the build.sh script](#12-using-the-buildsh-script)
  - [1.3. Removing ALL podman containers](#13-removing-all-podman-containers)
  - [1.4. Using build.sh](#14-using-buildsh)
    - [1.4.1. Building using an Ubuntu Container](#141-building-using-an-ubuntu-container)
    - [1.4.2. Building Alpine](#142-building-alpine)
    - [1.4.3. Building NetBSD](#143-building-netbsd)
      - [1.4.3.1. Build Failures from Ubuntu Jammy](#1431-build-failures-from-ubuntu-jammy)
      - [1.4.3.2. Using the Clang Toolchain](#1432-using-the-clang-toolchain)
    - [1.4.4. Building FreeBSD](#144-building-freebsd)
    - [1.4.5. Building QNX](#145-building-qnx)
- [2. Building using Makefile](#2-building-using-makefile)
  - [2.1. Overriding Targets](#21-overriding-targets)
- [3. Adding Your Own Distribution](#3-adding-your-own-distribution)

## 1. Podman Container - build.sh

This script uses `podman` to build a container image based on Ubuntu (of the
codename of your choosing), or other distributes where there is a docker file
that installs the necessary build tools.

It is used to simplify the testing of the repository against current and older
(supported) distributions.

### 1.1. Configuring Podman the First Time

If you've never used podman, these instructions provide a quick set up guide.
This is tested on Ubuntu 22.04, 26.04.

1. Install podman for your Ubuntu machine

   ```cmd
   # apt install podman
   ```

2. Check the `subuid` and `subgid` files.

   Check the contents of the file. You might see a default for your machine,
   here the default user is `user`:

   ```cmd
   # cat /etc/subuid
   user:100000:65536
   # cat /etc/subgid
   user:100000:65536
   ```

   If it's empty, then you have no defaults. Choose the first range. For
   example, if empty, then `100000-165535`, or in the example above,
   `200000-265535`.

   The `sub*` means "subordinate".

3. Add a new range. The user being added here is `jcurl`. Replace this with your
   own user.

   ```cmd
   # usermod --add-subuids 200000-265535 --add-subgids  200000-265535 jcurl
   ```

4. Check the updates were done.

   ```cmd
   # cat /etc/subuid
   user:100000:65536
   jcurl:200000:65536
   # cat /etc/subgid
   user:100000:65536
   jcurl:200000:65536
   ```

5. Finish the update

   ```cmd
   $ podman system migrate
   ```

### 1.2. Using the build.sh script

To see how to use the `build.sh` script, run the current version with the help
option.

```sh
./build.sh -?
```

It will provide you instructions on its use.

### 1.3. Removing ALL podman containers

If your machine is only uses podman for this repository, you can easily remove
all containers on podman with this instruction:

```sh
podman system prune --all --force && podman rmi --all --force
```

or

```sh
podman system reset
```

Running `make all` will reconstruct all repositories.

**WARNING**: Because the containers depend on external resources out side of
this repository, rebuilding may likely generate different images. The
Dockerfiles may not work! This is due to changes on how external resources
manage their URLs and package dependencies (e.g. with `alpine-latest` image,
packages may be refactored at any time, less likely with the `ubuntu-*` images).

### 1.4. Using build.sh

#### 1.4.1. Building using an Ubuntu Container

The `build.sh` script can be run that:

- Builds a `podman` container image from an Ubuntu `CODENAME` and installs the
  necessary tools inside.
- Runs the shell in the container in interactive mode
- Allows running as root (not the default)
- Allows running a specific command

For example:

```cmd
jcurl@localhost$ build.sh -cjammy -i
jcurl@ubuntu-jammy-06747d$ cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release
jcurl@ubuntu-jammy-06747d$ make -j8
jcurl@ubuntu-jammy-06747d$ make clangformat
```

Or to build on a single command line

```sh
./build.sh -cjammy "cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && make -j8"
```

Output is put in the folder `$REPO/qnx/build/ubuntu-CODENAME`.

Preconditions:

- Ensure that you've set up the `/etc/subuid` and `/etc/subgid` file first to
  allow `podman` to run as non-root.

Building the container removes unnecessary files to keep it small as possible.
If you want to test providing updates, you will need to run apt update first, to
populate the repository. Note, that running in root mode will let you modify the
container, but the `build.sh` script uses the `--rm` option to delete the
container when finished, so your results are not persisted.

#### 1.4.2. Building Alpine

Alpine is an alternative distribution with a smaller footprint. It always builds
against latest (so you'll need to remove the image and rebuild at periodic
points in time).

At this time, there are no automatic updates. Once built, it remains as it is
from the time it was build.

To build on a single command line, using LLVM:

```sh
make alpine-latest-clang
```

If you want to update the image, you must first destroy the image:

```sh
podman image rm coderesearch:alpine-latest
```

Then on the next invocation of the `Makefile`, the image will be regenerated.

#### 1.4.3. Building NetBSD

NetBSD supports a large number of target architectures. It is interesting as the
network stack `io-pkt` from QNX is similar to NetBSD.

To build the docker container, which builds the toolchain (may take 1h or
longer):

```sh
./build.sh -dnetbsd -c10.1
```

The `Makefile` and toolchain file `netbsd10.1-aarch64.cmake` and
`netbsd10.1-aarch64eb.cmake` depend on version 10.1.

The `netbsd` target in the `Makefile` builds for ARM 64-Bit Little Endian and
Big Endian.

The Big Endian is an architecture that is no longer very common, predominantly
from PowerPC and Motorola 68k processors. It has value to ensure the correctness
of a program, specifically network programming is done correctly.

Instructions used for building the NetBSD toolchain is taken from the [NetBSD
Guide - Building the
system](https://www.netbsd.org/docs/guide/en/chap-fetch.html). It should be
trivial to extend the Dockerfile to support other architectures (e.g. MIPS,
SH4), but ARM64 bit is chosen to compare on the Raspbery Pi 4.

##### 1.4.3.1. Build Failures from Ubuntu Jammy

The current podman/docker container pulls Ubuntu 24.04 (Noble). When building
from Ubuntu 24.04 or 26.04, the build of NetBSD toolchain works as expected.

When building from Ubuntu 22.04, permission errors occur.

The workaround is to build the podman container on Ubuntu 26.04, and then copy
it to an Ubuntu 22.04 machine.

After building on Ubuntu 26.04, back up the podman container as such:

```sh
podman save coderesearch:netbsd-10.1 | gzip -9 >netbsd10.1.tar.gz
```

Copy this to the Ubuntu 22.04 image, and restore it:

```sh
gunzip -c netbsd-10.1.tar.gz | podman load
```

Else you can modify the Docker container file `docker/netbsd-docker` to use
Ubuntu Jammy as the base image, which works. This results in an older version of
Clang-Tidy for tests and may not be as reliable as the one in Noble.

##### 1.4.3.2. Using the Clang Toolchain

There is a toolchain file for this project that uses the Clang toolchain from
Ubuntu Noble (clang 18.1.3). The advantage of using a clang toolchain is that is
allows seemless integration with the CMake implementation of clang-tidy. Thus,
the NetBSD sources can also be checked against clang-tidy.

#### 1.4.4. Building FreeBSD

FreeBSD is not as minimalistic as NetBSD and has a few forks and more software
support, but supports fewer targets. It is interesting as the network stack
`io-sock` from QNX 8.0 is similar to FreeBSD.

To build the docker container, which downloads the base files and extracts them
(the compiler toolchain comes from Ubuntu 22.04, which is Clang 14.0.0):

```sh
./build.sh -dfreebsd -c14.2
```

The `Makefile` and toolchain file `freebsd14.2-aarch64.cmake` depend on version
14.2.

#### 1.4.5. Building QNX

These instructions have been tested on QNX 7.1 and QNX 8.0. The QNX installation
is *mounted* into the container, and not copied. This speeds up significantly
the build and first run and reduces significantly the amount of space required
on your filesystem. It also means updates to the QNX installation via the QNX
software centre are mirrored in the container without having to update the
container.

Before running builds for QNX, you must set the the environment variable
`QNXSDP_$CODENAME` to the path where your QNX installation is (so that the file
`qnxsdp-env.sh` can be found).

For example, when building for QNX 7.1.0 and QNX 8.0.0, the parameter given to
`-cCODENAME` is used to identify the environment variable.

```sh
export QNXSDP_710=/home/${USER}/qnx/qnx710
export QNXSDP_800=/home/${USER}/qnx/qnx800
build.sh -dqnx -c710 -i
```

## 2. Building using Makefile

There is a `Makefile` in this directory, which allows executing all the `podman`
containers with the combinations of targets and different compilers to check for
compatibility.

From the `scripts` directory, run `make`. It will run `build.sh` to build the
containers, and execute each variant (GCC and Clang), with the output of the
build in `qnx/build/<dir>`. A container is only built the first time and
upstream changes are not automatically pulled. You will need to delete the image
and rebuild to obtain a new image.

Compilation stops on a build failure.

You can then run `build.sh` manually in interactive mode to debug and fix.

### 2.1. Overriding Targets

Buliding the entire repository multiple times per target may take too long. For
example, on a Skylake 6700T, it takes about 1.5 hours.

To reduce the time to test changes during development, two environment variables
are checked for on build.

```sh
TARGET=sdd make all qnx
```

This will build all containers, and build only the `sdd` binary. When providing
the `TARGET` environment variable, all tests are disabled by default - CTest
tries to run test cases that are defind but not built otherwise. When not
providing the `TARGET` variable, all tests are run by default.

If there are unit tests associated that should also run, we can extend this with
the TESTTARGET, which controls how `ctest` is run.

```sh
TARGET="sdd" TESTTARGET="libsjson_test ubench_test" make all qnx
```

This will build the targets (building `libsjson` doesn't build the test cases).
Targets defined for testing will run the tests.

Or if we're only building test cases to run them, the TARGET will automatically
take over the tests.

```sh
TESTTARGET="libsjson_test ubench_test" make all qnx
```

For those targets that can't run tests natively, the tests aren't run, but
they're built.

If you want to build all targets, but only test a subset, we need to use the
`ctest` terminology direct:

```sh
TESTTARGET="-R ubench_test|libsjson_test" make all qnx
```

## 3. Adding Your Own Distribution

Builds are done using a podman container under Linux. They should be reasonably
compatible (but not tested) with Docker.

Take your favourite distribution and add the packages that you need for being
able to build. Your distribution should ideally support rootless, that the
container is started with the same credentials as the user running inside the
container.

Create the file under `docker/$DISTRO-$CODENAME-docker`.

The `$DISTRO` is the name you want to give your distribution, it is normally the
name of the external distribution for ease.

The `$CODENAME` is normally the version of your distribution. Some use `latest`,
others `rolling`, or have a specific code name like `noble` for the Ubuntu
distribution.

These are provided on the command line to `build.sh` via the `-d$DISTRO` and
`-c$CODENAME`.

If the file `docker/$DISTRO-$CODENAME-docker` is not found, then the name
`docker/$DISTRO-docker` is used.

In all cases, the `$CODENAME` is passed as an argument to your docker definition
file.

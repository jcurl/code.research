# Scripts <!-- omit in toc -->

- [1. Podman Container - build.sh](#1-podman-container---buildsh)
  - [1.1. Using the build.sh script](#11-using-the-buildsh-script)
    - [1.1.1. Removing ALL podman containers](#111-removing-all-podman-containers)
  - [1.2. Using build.sh](#12-using-buildsh)
    - [1.2.1. Building Ubuntu](#121-building-ubuntu)
    - [1.2.2. Building Alpine](#122-building-alpine)
    - [1.2.3. Building NetBSD](#123-building-netbsd)
    - [1.2.4. Building FreeBSD](#124-building-freebsd)
    - [1.2.5. Building QNX](#125-building-qnx)
- [2. Building using Makefile](#2-building-using-makefile)
- [3. Adding Your Own Distribution](#3-adding-your-own-distribution)

## 1. Podman Container - build.sh

This script uses `podman` to build a container image based on Ubuntu (of the
codename of your choosing), or other distributes where there is a docker file
that installs the necessary build tools.

It is used to simplify the testing of the repository against current and older
(supported) distributions.

### 1.1. Using the build.sh script

To see how to use the `build.sh` script, run the current version with the help
option.

```sh
$ ./build.sh -?
```

It will provide you instructions on its use.

#### 1.1.1. Removing ALL podman containers

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

### 1.2. Using build.sh

#### 1.2.1. Building Ubuntu

The `build.sh` script can be run that:

- Builds a `podman` container image from an Ubuntu `CODENAME` and installs the
  necessary tools inside.
- Runs the shell in the container in interactive mode
- Allows running as root (not the default)
- Allows running a specific command

For example:

```sh
jcurl@localhost$ build.sh -cjammy -i
jcurl@180ae061e343$ cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release
jcurl@180ae061e343$ make -j8
jcurl@180ae061e343$ make clangformat
```

Or to build on a single command line

```sh
$ ./build.sh -cjammy "cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && make -j8"
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

#### 1.2.2. Building Alpine

Alpine is an alternative distribution with a smaller footprint. It always builds
against latest (so you'll need to remove the image and rebuild at periodic
points in time).

At this time, there are no automatic updates. Once built, it remains as it is
from the time it was build.

To build on a single command line, using LLVM:

```sh
$ make alpine-latest-clang
```

If you want to update the image, you must first destroy the image:

```sh
$ podman image rm coderesearch:alpine-latest
```

Then on the next invocation of the `Makefile`, the image will be regenerated.

#### 1.2.3. Building NetBSD

NetBSD supports a large number of target architectures. It is interesting as the
network stack `io-pkt` from QNX is similar to NetBSD.

To build the docker container, which builds the toolchain (may take 1h or
longer):

```sh
$ ./build.sh -dnetbsd -c10.1
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

#### 1.2.4. Building FreeBSD

FreeBSD is not as minimalistic as NetBSD and has a few forks and more software
support, but supports fewer targets. It is interesting as the network stack
`io-sock` from QNX 8.0 is similar to FreeBSD.

To build the docker container, which downloads the base files and extracts them
(the compiler toolchain comes from Ubuntu 22.04, which is Clang 14.0.0):

```sh
$ ./build.sh -dfreebsd -c14.2
```

The `Makefile` and toolchain file `freebsd14.2-aarch64.cmake` depend on version
14.2.

#### 1.2.5. Building QNX

These instructions have been tested on QNX 7.1 and QNX 8.0.

When building the container for the first time, you must first set the QNX
environment variables. These environment variables are necessary by the script
to find the folders and mount paths. When the container is built, the contents
of the QNX binaries in the host and target directories are *copied* into the
container.

You must have made sure prior that your QNX SDP works and you can compile the
sources manually.

After the container is built, the QNX environment variables are no longer needed
to be set before building. They will be set automatically inside the container.

```sh
$ . ~/qnx/qnx800/qnxsdp-env.sh
QNX_HOST=/home/jcurl/qnx/qnx800/host/linux/x86_64
QNX_TARGET=/home/jcurl/qnx/qnx800/target/qnx
MAKEFLAGS=-I/home/jcurl/qnx/qnx800/target/qnx/usr/include
```

The use the make file to build the image the first time:

```sh
$ make qnx-800-aarch64le
```

or build the container manually, and enter interactive mode.

```sh
$ ./build.sh -dqnx -c800 -i
```

During the build, the contents of where your QNX installation is copied into the
folder `./qnx/scripts/docker/qnx/tar`. This contains the binaries and host tools
that you've installed using the QNX software centre. If you destroy the
container and rebuild, this tarball is reused, speeding up the next build.

Alternatively, if you have a tarball from another installation, you can copy it
here with the same name (e.g. `qnx-800.tar.bz2`) and it will be extracted into
the container at `/opt/qnx`.

Because the QNX images are about 15-20GB (when almost everything is installed),
the initial creation of the containers are slow and can take a lot of memory
(~25GB with ~55GB needed during the container build). You might want to consider
your podman storage configuration.

The containers for QNX are configured to run as root (the `build.sh -r` option).
This speeds up considerably the first run of QNX, as it won't rewrite all the
overlay containers. See [Performance - Choosing a Storage
Driver](https://github.com/containers/podman/blob/1855765/docs/tutorials/performance.md#choosing-a-storage-driver).

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

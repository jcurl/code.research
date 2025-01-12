# Scripts <!-- omit in toc -->

- [1. Podman Container - build.sh](#1-podman-container---buildsh)
  - [1.1. Building Ubuntu](#11-building-ubuntu)
  - [1.2. Building Alpine](#12-building-alpine)
  - [1.3. Building NetBSD](#13-building-netbsd)
  - [1.4. Building FreeBSD](#14-building-freebsd)
  - [1.5. Using the build.sh script](#15-using-the-buildsh-script)
  - [1.6. Removing ALL podman containers](#16-removing-all-podman-containers)
- [2. Building using Makefile](#2-building-using-makefile)
- [3. Adding Your Own Distribution](#3-adding-your-own-distribution)

## 1. Podman Container - build.sh

This script uses `podman` to build a container image based on Ubuntu (of the
codename of your choosing), or other distributes where there is a docker file
that installs the necessary build tools.

It is used to simplify the testing of the repository against current and older
(supported) distributions.

### 1.1. Building Ubuntu

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

### 1.2. Building Alpine

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

### 1.3. Building NetBSD

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

### 1.4. Building FreeBSD

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

### 1.5. Using the build.sh script

To see how to use the `build.sh` script, run the current version with the help
option.

```sh
$ ./build.sh -?
```

It will provide you instructions on its use.

### 1.6. Removing ALL podman containers

If your machine is only uses podman for this repository, you can easily remove
all containers on podman with this instruction:

```sh
podman system prune --all --force && podman rmi --all --force
```

Running `make all` will reconstruct all repositories.

**WARNING**: Because the containers depend on external resources out side of
this repository, rebuilding may likely generate different images. The
Dockerfiles may not work! This is due to changes on how external resources
manage their URLs and package dependencies (e.g. with `alpine-latest` image,
packages may be refactored at any time, less likely with the `ubuntu-*` images).

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

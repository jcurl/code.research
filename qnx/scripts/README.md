# Scripts <!-- omit in toc -->

- [1. Podman Container - build.sh](#1-podman-container---buildsh)
  - [1.1. Building Ubuntu](#11-building-ubuntu)
  - [1.2. Building Alpine](#12-building-alpine)
  - [1.3. Using the build.sh script](#13-using-the-buildsh-script)
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
$ ./build.sh -dalpine -clatest -vclang "rm -rf * && cmake -B . -S /source/qnx -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/source/toolchain/x86-linux-clang.cmake && cmake --build . -j${NUM_CPUS}"
```

### 1.3. Using the build.sh script

To see how to use the `build.sh` script, run the current version with the help
option.

```sh
$ ./build.sh -?
```

It will provide you instructions on its use.

## 2. Building using Makefile

There is a `Makefile` in this directory, which allows executing all the podman
containers with the combinations of targets and different compilers to check for
compatibility.

From the `scripts` directory, run `make`. It will run `build.sh` to build the
containers, and execute each variant (GCC and Clang), with the output of the
build in `qnx/build/<dir>`.

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

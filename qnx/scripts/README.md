# Scripts

## Podman Container - build.sh

This script uses `podman` to build a container image based on Ubuntu (of the
codename of your choosing), installing the necessary build tools.

It is used to simplify the testing of the repository against current and older
(supported) Ubuntu variants.

The `build.sh` script can be run that:

- Builds a `podman` container image from an Ubuntu `CODENAME` and installs the
  necessary tools inside.
- Runs the shell in the container in interactive mode
- Allows running as root (not the default)
- Allows running a specific command

For example:

```cmd
jcurl@localhost$ build.sh -cjammy -i
jcurl@180ae061e343$ cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release
jcurl@180ae061e343$ make -j8
jcurl@180ae061e343$ make clangformat
```

Or to build on a single command line

```cmd
$ build.sh -cjammy "cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && make -j8"
```

Output is put in the folder `$REPO/qnx/build/ubuntu-CODENAME`.

Preconditions:

- Ensure that you've set up the `/etc/subuid` and `/etc/subgid` file first to allow
  `podman` to run as non-root.

Building the container removes unnecessary files to keep it small as possible.
If you want to test providing updates, you will need to run apt update first, to
populate the repository. Note, that running in root mode will let you modify the
container, but the `build.sh` script uses the `--rm` option to delete the
container when finished, so your results are not persisted.

## Building using Makefile

There is a `Makefile` in this directory, which allows executing all the podman
containers with the combinations of targets and different compilers to check for
compatibility.

From the `scripts` directory, run `make`. It will run `build.sh` to build the
containers, and execute each variant (GCC and Clang), with the output of the
build in `qnx/build/<dir>`.

If a build fails, then compilation stops.

You can then run `build.sh` manually in interactive mode to debug and fix.

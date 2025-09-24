#! /bin/bash

# The build script should allow us to run a command in the podman container
#
# build.sh [-c CODENAME] -i
# build.sh [-c CODENAME] "command"

BASEDIR=`realpath -e "$(dirname "$0")/../.." --relative-to=.`

DISTRO=ubuntu
VARIANT=
INTERACTIVE=0
ROOT=0
CODENAME=jammy
REBUILD=0
DEBUG=0

CODENAMESET=0
OPTSTRING="c:d:firv:D?"
while getopts $OPTSTRING OPTION; do
  case $OPTION in
  i)
    INTERACTIVE=1
    ;;
  c)
    CODENAME=$OPTARG
    CODENAMESET=1
    ;;
  d)
    DISTRO=$OPTARG
    if [ $CODENAMESET -eq 0 ]; then
      CODENAME=""
    fi
    ;;
  r)
    ROOT=1
    ;;
  v)
    VARIANT=$OPTARG
    ;;
  f)
    REBUILD=1
    ;;
  D)
    DEBUG=1
    REBUILD=1
    ;;
  ?)
    echo "build.sh -i [-r] [-d DISTRO] [-c CODENAME] [-v VAR]"
    echo "  Run in interactive mode for Ubuntu container \$CODENAME"
    echo ""
    echo "build.sh [-r] [-d DISTRO] [-c CODENAME] [-v VAR] 'build commands'"
    echo "  Run the command in the quotes. Quotes are required"
    echo "  to ensure your current shell doesn't interpret the"
    echo "  commands, but instead are sent to the container shell."
    echo ""
    echo "Options:"
    echo " -d - Choose a distribution \$DISTRO (docker file). Default is 'ubuntu'."
    echo " -c - Define the \$CODENAME for the \$DISTRO to build the container from"
    echo " -v - Define a build variant. Allows different builds for the same"
    echo "      \$CODENAME (e.g. clang, gcc, etc.). Only changes the build path."
    echo " -i - Run in interactive mode (don't run the commands at the end, but"
    echo "      start the shell)"
    echo " -r - Start in the container as root. Note, that the container is"
    echo "      dropped when finished, so changes are not permanent."
    echo ""
    echo " -f - Force rebuild. If the container exists, try to rebuild again anyway"
    echo " -D - Debug. Don't squash the container. Implies '-f' to force rebuild on"
    echo "      run. Useful when debugging and testing a Dockerfile."
    echo ""
    echo "Example:"
    echo "  $ build.sh -cjammy 'cmake -B . -S /source/qnx -DCMAKE_BUILD_TYPE=Release && make -j8'"
    echo ""
    echo "The container is started so that:"
    echo " /source - the repository (as read-only)"
    echo "           maps to \$REPO"
    echo " /build  - the work directory (as read-write)"
    echo "           maps to \$REPO/qnx/build/\$DISTRO-\$CODENAME-\$VARIANT"
    echo ""
    echo "You will need 'podman' installed, tested on Ubuntu 22.04, podman 3.4.4"
    exit 0
    ;;
  *)
    echo "Incorrect options provided"
    exit 1
    ;;
  esac
done
shift $((OPTIND-1))
OTHERARGS=$@

if [ "$CODENAME" == "" ]; then
  echo "Must set the code name."
  exit 1
fi
if [ "$DISTRO" == "" ]; then
  echo "Distro is not set."
  exit 1
fi

# If the user provides "latest", then this could be many different
# distributions. So rename it to not conflict.
PODVERSION=$DISTRO-$CODENAME
echo PODVERSION=coderesearch:$PODVERSION

# Check if we should build the podman image. We always rebuild if the user
# provides `-f` or `-D` (and this sets REBUILD=1)
if [ $REBUILD -eq 0 ]; then
  podman image inspect "coderesearch:$PODVERSION" > /dev/null 2> /dev/null
  REBUILD=$?
fi
if [ $REBUILD -ne 0 ]; then
  DOCKERFILE=$BASEDIR/qnx/scripts/docker/$PODVERSION-docker
  if [ ! -f "$DOCKERFILE" ]; then
    DOCKERFILE=$BASEDIR/qnx/scripts/docker/$DISTRO-docker
  fi
  if [ ! -f "$DOCKERFILE" ]; then
    echo "Can't find file 'docker/$PODVERSION-docker' to build"
    exit 1
  fi

  # If the user wants to debug, we don't squash. This lets them make
  # modifications to the docker file, and rebuild with the `-D` option, using the
  # layers that have not changed. Only at the end when the user is satisfied
  # everything works, the `-D` is not given and the build is squashed to save
  # space on disk.
  SQUASH=--squash
  if [ $DEBUG -ne 0 ]; then
    SQUASH=
  fi

  podman build $SQUASH --build-arg CODE_VERSION=$CODENAME -t "coderesearch:$PODVERSION" $BASEDIR/qnx/scripts/docker -f $DOCKERFILE
  if [ $? -ne 0 ]; then
    echo "Error building. Exiting"
    exit 1
  fi
fi

# The path `/source` is r/o unless running interactively, or as root. Podman
# appears to set the HOME directory depending on if we use "--userns=keep-id" or
# not.
#
# PODMAN_HOME - The home directory that podman sets up when running. Needed to
# know where to mount external volumes, depending on root-mode or rootless-mode.
#
# PODMAN_USERNAME - Podman doesn't provide the USER environment variable. Some
# build systems use the $USER to provide information (e.g. QNX).
if [ ${ROOT} -ne 0 ]; then
  ROOTOPT=""
  SOURCE="rw"
  PODMAN_HOME="/root"
  PODMAN_USERNAME="root($USER)"
else
  ROOTOPT="--userns=keep-id"
  if [ ${INTERACTIVE} -ne 0 ]; then
    SOURCE="rw"
  else
    SOURCE="ro"
  fi
  PODMAN_HOME="/home/user"
  PODMAN_USERNAME="$USER"
fi

# Create the build directory where to put the build results. We don't leave the
# results in the podman container, but outside, so that it can be inspected, or
# copied to the target.
BUILDDIR=""
if [ -z "$VARIANT" ]; then
  BUILDDIR="$PODVERSION"
else
  BUILDDIR="$PODVERSION-$VARIANT"
fi

if [ ! -e "$BASEDIR/qnx/build/$BUILDDIR" ]; then
  mkdir -p "$BASEDIR/qnx/build/$BUILDDIR"
fi

# Calculate a host name, that it is consistent. We give this hostname to the
# machines, else we get only a randomly generated hostname every time, which
# isn't helpful (our containers are only temporary)
#
# Ubuntu 22.04 has the file "machine-id" which is a unique string during build.
# We don't need it all, assuming it's random, just take the first six chars.
PODMAN_HOST=$PODVERSION-$(tr -d '[:space:]' < /etc/machine-id | cut -c 1-6)

# Set up any special mounts in the container when starting.
case $DISTRO in
qnx)
  # Determine where the user's SDP is. We mount this path, rather than copy it
  # into the container, as it can be quite large, making the build very slow.
  QNXSDP_VAR=`echo QNXSDP_${CODENAME}`
  if [ -z "${!QNXSDP_VAR}" ]; then
    echo The variable \'${QNXSDP_VAR}\' is empty. Ensure this is set before starting
    echo the container, and it is defined to where the \'qnxsdp-env.sh\' file can be
    echo found. Exiting.
    exit 1
  fi
  if [ ! -f "${!QNXSDP_VAR}/qnxsdp-env.sh" ]; then
    echo The file ${!QNXSDP_VAR}/qnxsdp-env.sh doesn\'t exist. Exiting.
    exit 1
  fi
  PODMAN_MOUNT="-v ${!QNXSDP_VAR}:/opt/qnx:ro"

  # Under QNX we need to mount the configuration file, so that the licensing
  # path can be found.
  if [ -z "$QNX_CONFIGURATION" ]; then
    PODMAN_MOUNT="$PODMAN_MOUNT -v $HOME/.qnx:$PODMAN_HOME/.qnx"
  else
    REL=$(realpath --relative-to="$HOME" "$QNX_CONFIGURATION")
    PODMAN_MOUNT="$PODMAN_MOUNT -v $QNX_CONFIGURATION:$PODMAN_HOME/$REL:rw"
  fi
  echo "PODMAN_MOUNT      = $PODMAN_MOUNT"
  ;;
esac

# Run the image
if [ ${INTERACTIVE} -ne 0 ]; then
  podman run -it --rm $ROOTOPT -e USER=$PODMAN_USERNAME -h $PODMAN_HOST -v $PWD/$BASEDIR:/source:${SOURCE} -v "$PWD/$BASEDIR/qnx/build/$BUILDDIR":/build:rw $PODMAN_MOUNT --tmpfs /tmp "coderesearch:$PODVERSION"
else
  if [ "x$OTHERARGS" = "x" ]; then
    echo "No command, doing nothing..."
  else
    echo "Non-Interactive"
    echo "$OTHERARGS"
    podman run -t --rm $ROOTOPT -e USER=$PODMAN_USERNAME -h $PODMAN_HOST -v $PWD/$BASEDIR:/source:${SOURCE} -v "$PWD/$BASEDIR/qnx/build/$BUILDDIR":/build:rw $PODMAN_MOUNT --tmpfs /tmp "coderesearch:$PODVERSION" sh -l -c "$OTHERARGS"
  fi
fi

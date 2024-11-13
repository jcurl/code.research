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
CODENAME=focal

OPTSTRING="c:d:irv:?"
while getopts ${OPTSTRING} OPTION; do
  case $OPTION in
  i)
    INTERACTIVE=1
    ;;
  c)
    CODENAME=$OPTARG
    ;;
  d)
    DISTRO=$OPTARG
    if [ -z "${DISTRO}" ]; then
      echo "Must provide a distribution"
      exit 1
    fi
    if [ ! -f "${BASEDIR}/qnx/scripts/docker/${DISTRO}-docker" ]; then
      echo "Can't find qnx/scripts/docker/${DISTRO}-docker"
      exit 1
    fi
    ;;
  r)
    ROOT=1
    ;;
  v)
    VARIANT=$OPTARG
    ;;
  ?)
    echo "build.sh -i [-r] [-c CODENAME]"
    echo "  Run in interactive mode for Ubuntu container \$CODENAME"
    echo ""
    echo "build.sh [-r] [-c CODENAME] 'build commands'"
    echo "  Run the command in the quotes. Quotes are required"
    echo "  to ensure your current shell doesn't interpret the"
    echo "  commands, but instead are sent to the container shell."
    echo ""
    echo "Options:"
    echo " -c - Define the Ubuntu \$CODENAME to build the container from"
    echo " -d - Choose a distribution \$DISTRO (docker file). Default is 'ubuntu'."
    echo " -i - Run in interactive mode (don't run the commands at the end, but"
    echo "      start the shell)"
    echo " -r - Start in the container as root. Note, that the container is"
    echo "      dropped when finished, so changes are not permanent."
    echo " -v - Define a build variant. Allows different builds for the same"
    echo "      \$CODENAME (e.g. clang, gcc, etc.). Only changes the build path."
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

echo CODENAME=${CODENAME}

podman image inspect "coderesearch:${CODENAME}" > /dev/null 2> /dev/null
if [ $? -ne 0 ]; then
  podman build --build-arg CODE_VERSION=${CODENAME} -t "coderesearch:${CODENAME}" $BASEDIR/qnx/scripts/docker -f $BASEDIR/qnx/scripts/docker/${DISTRO}-docker
  if [ $? -ne 0 ]; then
    echo "Error building. Exiting"
    exit 1
  fi
fi

if [ ${ROOT} -ne 0 ]; then
  ROOTOPT=""
else
  ROOTOPT="--userns=keep-id"
fi

VARIANTEXT=""
if [ -z "${VARIANT}" ]; then
  VARIANTEXT="${CODENAME}"
else
  VARIANTEXT="${CODENAME}-${VARIANT}"
fi

if [ ! -e "${BASEDIR}/qnx/build/${DISTRO}-${VARIANTEXT}" ]; then
  mkdir -p "${BASEDIR}/qnx/build/${DISTRO}-${VARIANTEXT}"
fi

if [ ${INTERACTIVE} -ne 0 ]; then
  podman run -it --rm ${ROOTOPT} -v ${PWD}/${BASEDIR}:/source:ro -v "${PWD}/${BASEDIR}/qnx/build/${DISTRO}-${VARIANTEXT}":/build:rw --tmpfs /tmp "coderesearch:${CODENAME}"
else
  echo "Non-Interactive"
  echo "${OTHERARGS}"
  podman run -t --rm ${ROOTOPT} -v ${PWD}/${BASEDIR}:/source:ro -v "${PWD}/${BASEDIR}/qnx/build/${DISTRO}-${VARIANTEXT}":/build:rw --tmpfs /tmp "coderesearch:${CODENAME}" sh -c "${OTHERARGS}"
fi

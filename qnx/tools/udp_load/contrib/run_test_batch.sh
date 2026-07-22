#! /bin/sh

OPTSTRING="S:D:"
SOURCE=""
DESTINATION=""
while getopts $OPTSTRING OPTION; do
  case $OPTION in
  S)
    SOURCE=$OPTARG
    ;;
  D)
    DESTINATION=$OPTARG
    ;;
  *)
    echo "Incorrect options provided"
    exit 1
    ;;
  esac
done

if [ "x${DESTINATION}" = "x" ]; then
  echo "-D<destination> is required"
  exit 1
fi
if [ "x${SOURCE}" = "x" ]; then
  echo "-S<source> is required"
  exit 1
fi

echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
uname -a

echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
./run_test.sh -S $SOURCE -D $DESTINATION -B sendto

echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
./run_test.sh -S $SOURCE -D $DESTINATION -B sendmmsg

echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
./run_test.sh -S $SOURCE -D $DESTINATION -B bpf

echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
./run_test.sh -S $SOURCE -D $DESTINATION -B bpfmm

echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
./run_test.sh -S 127.0.0.1 -D 127.0.0.1 -B sendto

echo ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
./run_test.sh -S 127.0.0.1 -D 127.0.0.1 -B sendmmsg

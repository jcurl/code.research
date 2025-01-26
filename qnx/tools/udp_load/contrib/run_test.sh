#! /bin/sh

OPTSTRING="S:D:B:"
SOURCE=""
DESTINATION=""
MODE="sendto"
while getopts $OPTSTRING OPTION; do
  case $OPTION in
  S)
    SOURCE=$OPTARG
    ;;
  D)
    DESTINATION=$OPTARG
    ;;
  B)
    MODE=$OPTARG
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

for t in 1 2 3 4; do
  for p in 500 1000 1500 2000 2500 3000 3500 4000 4500 5000 5500 6000 6500 7000 7500 8000; do
    echo =======================================
    ./udp_load -I -S $SOURCE -D $DESTINATION -B $MODE -T$t -p$p
    sleep 2
  done
done

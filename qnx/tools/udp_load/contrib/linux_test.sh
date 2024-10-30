#! /bin/bash

for (( t=1; t<=4; t++ )) do
  for (( p=500; p<9000; p+=500 )); do
    echo =======================================
    ./tools/udp_load/udp_load -I -S192.168.1.110 -D192.168.1.112 -T$t -p$p
    sleep 2
  done
done

for (( t=1; t<=4; t++ )) do
  for (( p=500; p<9000; p+=500 )); do
    echo =======================================
    ./tools/udp_load/udp_load -I -S127.0.0.1 -D127.0.0.1 -T$t -p$p
    sleep 2
  done
done

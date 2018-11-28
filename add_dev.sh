#!/bin/bash

base_path="/dev/queue"
make clean &&
make &&
sleep 1 &&
insmod pri_que.ko queue_nr_devs=$1 &&
echo "insmod finished"
sleep 1 &&
for (( c=0; c<$(($1)); c++ ))
do  
    mknod "$base_path$c" c $(grep queue /proc/devices | grep [0123456789] | cut -d' ' -f1) $c
done 

dmesg -c &&
# clear
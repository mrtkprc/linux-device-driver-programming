#!/bin/bash

make clean &&
make &&
sleep 1 &&
insmod pri_que.ko &&
mknod /dev/queue0 c $(grep queue /proc/devices | grep [0123456789] | cut -d' ' -f1) 0

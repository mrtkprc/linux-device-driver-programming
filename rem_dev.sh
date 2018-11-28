#!/bin/bash
base_path="/dev/queue"
rmmod pri_que &&

for (( c=0; c<$(($1)); c++ ))
do  
    rm "$base_path$c"
done 
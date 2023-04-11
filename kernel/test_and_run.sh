#!/bin/bash

make
insmod ./weekernel.ko
sleep 1
rmmod weekernel

dmesg
make clean

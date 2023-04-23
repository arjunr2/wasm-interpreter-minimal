#!/bin/bash

kmod_name=wasmkernel

make
insmod ./$kmod_name.ko
sleep 1
rmmod $kmod_name

dmesg
make clean

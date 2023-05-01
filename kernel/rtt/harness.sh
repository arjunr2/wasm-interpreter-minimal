#!/bin/bash

make
dmesg -c
rmmod send_rtt
insmod ./send_rtt.ko


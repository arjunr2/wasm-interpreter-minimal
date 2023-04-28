#!/bin/bash
make
insmod ./recv_msg.ko
sleep 1
insmod ./send_msg.ko
sleep 3
rmmod send_msg.ko
rmmod recv_msg.ko


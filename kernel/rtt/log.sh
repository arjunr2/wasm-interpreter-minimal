#!/bin/bash

dmesg | grep "RTT Time" | cut -d':' -f2 | paste -d',' -s &> pingpong.results

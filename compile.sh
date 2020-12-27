#!/bin/bash
make clean && make all && insmod ./asd.ko && mknod /dev/scull c 240 0 && grep scull /proc/devices
#!/bin/bash
make clean && make all && insmod ./vault.ko && mknod /dev/vault c 240 0 && grep vault /proc/devices
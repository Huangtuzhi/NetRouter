#!/bin/ash
mknod /dev/ed_rec c 200 0
mknod /dev/ed_tx  c 201 0
insmod ed_device.ko
lsmod
ifconfig ed0 192.168.5.1 up

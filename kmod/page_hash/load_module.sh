#!/bin/bash
#Load the module and create the char dev *and redo everything if it already
#exists

NODENAME="node"
DEVNAME="my_char_dev"
MODNAME="page_hash"
[[ -c $NODENAME ]] && rm $NODENAME
[[ `lsmod | grep ${MODNAME}` ]] && rmmod ./${MODNAME}.ko
insmod ./${MODNAME}.ko
mknod $NODENAME c `grep $DEVNAME /proc/devices | cut -f1 -d' '` 0

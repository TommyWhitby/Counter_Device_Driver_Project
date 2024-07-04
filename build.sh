#!/usr/bin/env bash

# check if counter devices already exist and remove
if [ $(ls /dev | grep counter | wc -l) -gt 0 ]
then
    sudo rm /dev/counter[0-9]*
    echo "Removed existing counter devices"
fi
# check if counter module is inserted and remove
if [ $(lsmod | grep counter | wc -l) -gt 0 ]
then
    sudo rmmod counter
    echo "Removed existing counter module"
fi

# make module/device driver, exit if failure (compilation error)
make || { echo "Build aborted: Make error";  exit 1; }

# insert module
sudo insmod ./counter.ko

sudo bash -c "echo 8 > /proc/sys/kernel/printk"
sudo bash -c "echo 'file counter.c +p' > /sys/kernel/debug/dynamic_debug/control"

# major device number is dynamically allocated
maj=$(cat /proc/devices | grep counter | cut -d ' ' -f1)

# iterate over the number of minor devices and create a node
for min in $(seq 0 3); do
    sudo mknod /dev/counter${min} c ${maj} ${min} -m 666
done

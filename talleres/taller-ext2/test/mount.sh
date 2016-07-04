#! /bin/bash

sudo losetup -o 2097152 /dev/loop1 hdd.raw
sudo mount -t ext2 /dev/loop1 hdd_dir

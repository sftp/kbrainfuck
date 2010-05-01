#!/bin/sh

make
rmmod kbrainfuck
insmod ./kbrainfuck.ko

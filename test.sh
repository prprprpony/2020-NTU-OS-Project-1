#!/bin/bash
gcc main.c -o main
sudo dmesg -C
./main < $1
dmesg | grep Project1


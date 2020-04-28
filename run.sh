#!/bin/bash
gcc main.c -o main
mkdir output
for p in "FIFO" "RR" "SJF" "PSJF"; do 
  echo $p
  for ((i = 1; i <= 5; ++i)); do
    fin="$p""_$i.txt"
    fout="$p""_$i""_stdout.txt"
    fdmesg="$p""_$i""_dmesg.txt"
    #echo $fin $fout $fdmesg
    sudo dmesg -C
    touch output/$fstdout
    touch output/$fdmesg
    ./main < OS_PJ1_Test/$fin > output/$fout
    dmesg | grep Project1 > output/$fdmesg
  done
done


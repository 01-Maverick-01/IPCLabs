#!/bin/bash
path1=./lab1_output
path2=./lab4_output
things="2-2 4-4 8-8 16-16 32-32 64-64"
mkdir diff
for n in $(echo $things); do
    diff $path1/x1/current.out $path2/$n/x1/current.out > ./diff/serial-$n-x1.txt
    cat ./diff/serial-$n-x1.txt | wc -l
    
    diff $path1/00100/current.out $path2/$n/00100/current.out > ./diff/serial-$n-00100.txt
    cat ./diff/serial-$n-00100.txt | wc -l

    diff $path1/01000/current.out $path2/$n/01000/current.out > ./diff/serial-$n-01000.txt
    cat ./diff/serial-$n-01000.txt | wc -l

    diff $path1/05000/current.out $path2/$n/05000/current.out > ./diff/serial-$n-05000.txt
    cat ./diff/serial-$n-05000.txt | wc -l

    diff $path1/10000/current.out $path2/$n/10000/current.out > ./diff/serial-$n-10000.txt
    cat ./diff/serial-$n-10000.txt | wc -l
done



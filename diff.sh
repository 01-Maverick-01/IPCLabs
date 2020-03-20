#!/bin/bash

things="serial 2-2 4-4 8-8 16-16 32-32"
for n in $(echo $things); do
    diff ./1-1/x1/current.out ./$n/x1/current.out > ./diff/1-$n-x1.txt
    cat ./diff/1-$n-x1.txt | wc -l
    
    diff ./1-1/00100/current.out ./$n/00100/current.out > ./diff/1-$n-00100.txt
    cat ./diff/1-$n-00100.txt | wc -l

    diff ./1-1/01000/current.out ./$n/01000/current.out > ./diff/1-$n-01000.txt
    cat ./diff/1-$n-01000.txt | wc -l

    diff ./1-1/05000/current.out ./$n/05000/current.out > ./diff/1-$n-05000.txt
    cat ./diff/1-$n-05000.txt | wc -l

    diff ./1-1/10000/current.out ./$n/10000/current.out > ./diff/1-$n-10000.txt
    cat ./diff/1-$n-10000.txt | wc -l
done



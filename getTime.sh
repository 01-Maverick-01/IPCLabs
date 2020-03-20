#!/bin/bash

path=./lab3_output
things="2-2 4-4 8-8 16-16 32-32 64-64"
for n in $(echo $things); do
    echo $n-x1
    cat $path/$n/x1/prod_cons_single_thread.e* | grep "real"
    cat $path/$n/x1/prod_cons_single_thread.e* | grep "user"
    
    echo $n-100
    cat $path/$n/00100/prod_cons_single_thread.e* | grep "real"
    cat $path/$n/00100/prod_cons_single_thread.e* | grep "user"

    echo $n-1000
    cat $path/$n/01000/prod_cons_single_thread.e* | grep "real"
    cat $path/$n/01000/prod_cons_single_thread.e* | grep "user"

    echo $n-5000
    cat $path/$n/05000/prod_cons_single_thread.e* | grep "real"
    cat $path/$n/05000/prod_cons_single_thread.e* | grep "user"

    echo $n-10000
    cat $path/$n/10000/prod_cons_single_thread.e* | grep "real"
    cat $path/$n/10000/prod_cons_single_thread.e* | grep "user"
done



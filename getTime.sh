#!/bin/bash

things="serial 1-1 2-2 4-4 8-8 16-16 32-32"
for n in $(echo $things); do
    echo $n-x1
    cat ./$n/x1/prod_cons_single_thread.e* | grep "real"
    cat ./$n/x1/prod_cons_single_thread.e* | grep "user"
    
    echo $n-100
    cat ./$n/00100/prod_cons_single_thread.e* | grep "real"
    cat ./$n/00100/prod_cons_single_thread.e* | grep "user"

    echo $n-1000
    cat ./$n/01000/prod_cons_single_thread.e* | grep "real"
    cat ./$n/01000/prod_cons_single_thread.e* | grep "user"

    echo $n-5000
    cat ./$n/05000/prod_cons_single_thread.e* | grep "real"
    cat ./$n/05000/prod_cons_single_thread.e* | grep "user"

    echo $n-10000
    cat ./$n/10000/prod_cons_single_thread.e* | grep "real"
    cat ./$n/10000/prod_cons_single_thread.e* | grep "user"
done



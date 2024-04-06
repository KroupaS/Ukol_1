#!/bin/bash

mpicc -O3 -Wall main.c solve.c board.c -fopenmp -lrt -o vps.out

success_all=true

for file in $(ls -1 in_*.txt | sort); do
    # Run the program on each file
    mpirun -np 12 ./vps.out "$file" -s

    if [ $? -ne 0 ]; then
	success_all=false
    fi
done

if [ "$success_all" = true ]; then
    echo "All runs successful"
else
    echo "Failure"
fi

exit 0

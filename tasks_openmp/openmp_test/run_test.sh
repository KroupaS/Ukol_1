#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <X>"
    exit 1
fi

X="$1"

output_file="./openmp_test/depth${X}.out"

for ((i=1; i<=10; i++)); do
    # Launch the program with parameter i
    ./vps.out 1 in_0000.txt >> "$output_file" 2>&1
done
for ((i=1; i<=10; i++)); do
    # Launch the program with parameter i
    ./vps.out 1 in_0001.txt >> "$output_file" 2>&1
done
for ((i=1; i<=10; i++)); do
    # Launch the program with parameter i
    ./vps.out 1 in_0002.txt >> "$output_file" 2>&1
done
for ((i=1; i<=10; i++)); do
    # Launch the program with parameter i
    ./vps.out 1 in_0009.txt >> "$output_file" 2>&1
done
for ((i=1; i<=10; i++)); do
    # Launch the program with parameter i
    ./vps.out 1 in_0011.txt >> "$output_file" 2>&1
done
for ((i=1; i<=10; i++)); do
    # Launch the program with parameter i
    ./vps.out 1 in_0015.txt >> "$output_file" 2>&1
done
for ((i=1; i<=10; i++)); do
    # Launch the program with parameter i
    ./vps.out 1 in_0016.txt >> "$output_file" 2>&1
done
for ((i=1; i<=10; i++)); do
    # Launch the program with parameter i
    ./vps.out 1 in_0017.txt >> "$output_file" 2>&1
done

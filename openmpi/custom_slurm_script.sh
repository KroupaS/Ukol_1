#!/bin/bash


success_all=true

# Aktivace HPE CPE
source /etc/profile.d/zz-cray-pe.sh

export MV2_HOMOGENEOUS_CLUSTER=1
export MV2_SUPPRESS_JOB_STARTUP_PERFORMANCE_WARNING=1

# Nastavení proměnných prostředí pro naplánovanou úlohu
#module load cray-mvapich2_pmix_nogpu/2.3.7
module load cray-mvapich2_pmix_nogpu

export MV2_ENABLE_AFFINITY=0

cc -O3 -Wall main.c solve.c board.c -fopenmp -lrt -o vps.out

for file in $(ls -1 in_*.txt | sort); do
    # Run the program on each file
    srun -p arm_long -N 4 ./vps.out "$file" -s
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

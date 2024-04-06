#!/bin/bash

# Nastavení řídících parametrů plánovače Slurm
#SBATCH --job-name=my_mpi_program
#SBATCH --output="%x-%J.out"
#SBATCH --error="%x-%J.err"
#SBATCH --exclusive

# Aktivace HPE CPE
source /etc/profile.d/zz-cray-pe.sh

export MV2_HOMOGENEOUS_CLUSTER=1
export MV2_SUPPRESS_JOB_STARTUP_PERFORMANCE_WARNING=1

# Nastavení proměnných prostředí pro naplánovanou úlohu
module load cray-mvapich2_pmix_nogpu
export MV2_ENABLE_AFFINITY=0

# Run the program on each file
srun ./vps.out in_0001.txt -s

exit 0

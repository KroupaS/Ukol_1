#!/bin/bash

# Nastavení řídících parametrů plánovače Slurm
#SBATCH --job-name=my_mpi_program
#SBATCH --output="slurm_out/%x-%J.out"
#SBATCH --error="slurm_out%x-%J.err"

# Aktivace HPE CPE
source /etc/profile.d/zz-cray-pe.sh

# Nastavení proměnných prostředí pro naplánovanou úlohu
#module load cray-mvapich2_pmix_nogpu/2.3.7
#module load cray-mvapich2_pmix_nogpu

# Za příkazem srun napsat cestu k programu i s jeho argumenty, který se má spustit na naplánovaných výpočetních uzlech:
srun ./vps.out in_0001.txt -s 1

exit 0
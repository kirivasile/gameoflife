#!/bin/bash
#SBATCH --time=30
#SBATCH -i input.txt
/usr/lib64/openmpi/bin/mpirun --map-by ppr:1:node ./mpi.out

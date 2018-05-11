#!/bin/bash
#SBATCH -o $HOME/%A_%a.out
#SBATCH -e $HOME/%A_%a.err
#SBATCH -p test 
#SBATCH -c 2
#SBATCH -n 2
#SBATCH --mem-per-cpu=4500M
#SBATCH -t 480
#SBATCH --array=1-5

module load gcc/6.3.0-fasrc01 openmpi/2.1.0-fasrc01 java/1.8.0_45-fasrc01
mpirun -np 2 sampler InputFile.csv "${SLURM_ARRAY_TASK_ID}"

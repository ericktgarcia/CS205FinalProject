#!/bin/bash
#SBATCH -o /home/zjw2/sampler/mpiOut/%A_%a.out
#SBATCH -e /home/zjw2/sampler/mpiOut/%A_%a.err
#SBATCH -p short
#SBATCH -n 8
#SBATCH --mem-per-cpu=4500M
#SBATCH -t 720
#SBATCH --array=1-1000

module load gcc openmpi java
mpirun -np 8 sampler InputFile.csv "${SLURM_ARRAY_TASK_ID}"
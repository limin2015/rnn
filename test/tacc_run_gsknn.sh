#!/bin/bash
#SBATCH -J gsknn_job
#SBATCH -o gsknn_output.txt
#SBATCH -p gpu
#SBATCH -t 00:03:00
#SBATCH -n 1
#SBATCH -N 1
export OMP_NUM_THREADS=10

ibrun tacc_affinity run_gsknn.sh

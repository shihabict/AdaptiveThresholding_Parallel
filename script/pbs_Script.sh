#!/bin/bash
#PBS -N adaptive_thresh_hybrid
#PBS -l nodes=4:ppn=4        # 4 nodes, 4 cores per node (Picocluster nodes)
#PBS -l walltime=00:10:00
#PBS -j oe

cd "$PBS_O_WORKDIR"

module load mpi        # whatever your cluster uses
module load gcc        # if needed

export OMP_NUM_THREADS=4   # use 4 OpenMP threads per MPI rank

mpirun -np 4 ./main_hybrid \
    highway.pgm output.pgm 15 5 timing_hybrid.csv

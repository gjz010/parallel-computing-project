#!/bin/bash
echo RandomReduction Test
echo ">>>>>>>>>>> Building..."
make rr rr_dist/Reduction rr_dist/Validation_mpi RandomGen
echo ">>>>>>>>>>> Generating matrix..."
./RandomGen 1000 10000
echo ">>>>>>>>>>> Running on CPU..."
rr_dist/Reduction 1000 10000 -d -t
echo ">>>>>>>>>>> Running on MPI..."
mpirun -n 4 ./rr 1000 10000
echo ">>>>>>>>>>> Validating..."
rr_dist/Validation_mpi -p
rm *.mat matrix
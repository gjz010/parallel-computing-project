#!/bin/bash
R=10
C=100000
echo RandomReduction Test
echo ">>>>>>>>>>> Building..."
make rr rr_dist/Reduction rr_dist/Validation_mpi RandomGen
echo ">>>>>>>>>>> Generating matrix..."
./RandomGen $R $C
echo ">>>>>>>>>>> Running on CPU..."
rr_dist/Reduction $R $C -d -t
echo ">>>>>>>>>>> Running on MPI..."
if type bsub
then
echo "bsub detected. Assuming Sunway."
bsub -I -q q_sw_expr -n 16 ./rr $R $C
else
echo "bsub not detected. Using mpirun."
mpirun -n 4 ./rr $R $C
fi

echo ">>>>>>>>>>> Validating..."
rr_dist/Validation_mpi -p
rm *.mat matrix

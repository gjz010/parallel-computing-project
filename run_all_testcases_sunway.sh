#!/bin/bash
echo "MPI Benchmarking Project (Sunway Version)"
echo "Building"
make all
echo "Running Bench 1 (Point-to-Point Communication)"
bsub -I -q q_sw_expr -n 8 ./bench1 -c 10 -s 10000000 -g 0,1,2,3:4,5,6,7 | tee bench1.log
echo "Running Bench 2 (Collective Communication)"
bsub -I -q q_sw_expr -n 8 ./bench2 bcast,1000000,0,1,2,3 gather,1000000,4,5,6,7 reduce,1000000,0,2,4,6 allreduce,1000000,1,3,5,7 scan,1000000,1,2,5,7 alltoall,1000000,0,1,2,3,4,5,6,7 | tee bench2.log
echo "Running Bench 3 (MPIIO Benchmarking)"
bsub -I -q q_sw_expr -N 4 -np 2 ./bench3 bench3.mat c,0,10000000,1,10000000,2,10000000,3,10000000,4,10000000,5,10000000,6,10000000,7,10000000 \
    c,0,10000000,2,10000000,4,10000000,6,10000000 \
    w,0,10000000:w,1,10000000:w,2,10000000:w,3,10000000:w,4,10000000:w,5,10000000:w,6,10000000:w,7,10000000 \
    r,0,10000000:w,1,10000000:r,2,10000000:w,3,10000000:r,4,10000000:w,5,10000000:r,6,10000000:w,7,10000000 | tee bench3.log
echo "Running RandomReduction"
./rr_test.sh | tee bench4.log
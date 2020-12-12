#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "vec.h"
#include "utils.h"
#define ALL_THREADS
#define MAIN_THREAD if(world_rank==0)
#define SUB_THREADS if(world_rank!=0)



int mpi_main(int argc, char** argv){
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    (void)argc;
    (void)argv;
    return 0;
}
int main(int argc, char** argv){
    MPI_Init(NULL, NULL);
    int ret=mpi_main(argc, argv);
    MPI_Finalize();
    return ret;
}

#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include "vec.h"
#include "utils.h"

int fork_communicator(int world_rank, int* nodes, ull node_count, MPI_Comm* new_comm){
    int selected=0;
    int key = 0;
    for(ull i=0; i<node_count; i++){
        if(nodes[i]==world_rank){
            key = (int)i;
            selected=1;
            break;
        }
    }
    if(selected){
        MPI_Comm_split(MPI_COMM_WORLD, 1, key, new_comm);
        int new_rank=0;
        MPI_Comm_rank(*new_comm, &new_rank);
        return new_rank;
    }else{
        MPI_Comm_split(MPI_COMM_WORLD, MPI_UNDEFINED, 0, new_comm);
        return -1;
    }
}

typedef void(*BENCHMARK)(int, int, int*, ull);

#define _CONCAT(a,b) a##b
#define CONCAT(a,b) _CONCAT(a,b)
#define BENCH(x) CONCAT(bench_,x)
#define DEF_BENCH(x) void BENCH(x)(int world_rank, int size, int* nodes, ull node_count)
#define BENCH_DEFAULT do{(void)world_rank; (void)size; (void)nodes; (void)node_count;}while(0)
#define ROOT_THREAD if(rank==0)
#define LEAF_THREADS if(rank!=0)
DEF_BENCH(bcast){
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Comm new_comm;
    int rank;
    if((rank=fork_communicator(world_rank, nodes, node_count, &new_comm))!=-1){
        printf("%d is %d\n", world_rank, rank);

        MPI_Barrier(new_comm);
        MPI_Comm_free(&new_comm);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    
    BENCH_DEFAULT;
}
DEF_BENCH(gather){
    BENCH_DEFAULT;
}
DEF_BENCH(reduce){
    BENCH_DEFAULT;
}
DEF_BENCH(allreduce){
    BENCH_DEFAULT;
}
DEF_BENCH(scan){
    BENCH_DEFAULT;
}
DEF_BENCH(alltoall){
    BENCH_DEFAULT;
}
#define MAIN_THREAD if(world_rank==0)
#define SUB_THREADS if(world_rank!=0)
#define ALL_THREADS
int mpi_main(int argc, char** argv){
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    ull schedule_size = 0;
    int* schedule = NULL;
    int error_flag=0;
    MAIN_THREAD {
        vec_int v=NEW_VEC;
        if(argc==1){
            goto usage;
        }
        for(int i=1; i<argc; i++){
            char* s1;
            char* s_group = strtok_r(argv[i], ",", &s1);
            int flag=0;
            while(s_group){
                if(flag==0){
                    int type=-1;
                    if(strcmp(s_group, "bcast")==0){
                        type=0;
                    }
                    if(strcmp(s_group, "gather")==0){
                        type=1;
                    }
                    if(strcmp(s_group, "reduce")==0){
                        type=2;
                    }
                    if(strcmp(s_group, "allreduce")==0){
                        type=3;
                    }
                    if(strcmp(s_group, "scan")==0){
                        type=4;
                    }
                    if(strcmp(s_group, "alltoall")==0){
                        type=5;
                    }
                    if(type==-1){
                        fprintf(stderr, "Bad type\n");
                        goto bad_argument;
                    }
                    vec_push(&v, type);
                    flag++;
                }else if(flag==1){ 
                    int size;
                    int ret=sscanf(s_group, "%d", &size);
                    if(ret!=1){
                        goto bad_argument;
                    }
                    if(size<0){
                        goto bad_argument;
                    }
                    vec_push(&v, size);
                    flag++;
                }else {
                    int next;
                    int ret=sscanf(s_group, "%d", &next);
                    if(ret!=1){
                        goto bad_argument;
                    }
                    if(next<0 || next>=world_size){
                        fprintf(stderr, "Bad node\n");
                        goto bad_argument;
                    }
                    vec_push(&v, next);
                    flag++;
                }
                s_group=strtok_r(NULL, ",", &s1);
            }
            if(flag<3){
                fprintf(stderr, "The size and at least one node should be specified.\n");
                goto bad_argument;
            }
            vec_push(&v, -1);
            schedule_size = v.size;
            schedule = v.data;
        }
    }
    goto no_error;
bad_argument:
    fprintf(stderr, "Parsing argument error: bad argument!\n");
usage:
    fprintf(stderr, "MPI Bandwidth Tester #2\nusage: %s type,size,node0,node1,...,nodek\n \"type\" can be one of the following: bcast,gather,reduce,allreduce,scan,alltoall\n", argv[0]);
    error_flag=1;
no_error:
    ALL_THREADS {
        MPI_Bcast(&schedule_size, 1, MPI_LONG_LONG_INT, 0, MPI_COMM_WORLD);
        MAIN_THREAD{
            if(schedule_size==0){
                return error_flag;
            }
        }
        SUB_THREADS{
            if(schedule_size==0){return 0;}
            schedule = malloc(sizeof(int) * schedule_size);
        }
        MPI_Bcast(schedule, (int)schedule_size, MPI_INT, 0, MPI_COMM_WORLD);
        //printf("%d=%lld %d %d %d %d\n", world_rank, schedule_size, schedule[0], schedule[1], schedule[2], schedule[3]);
    }
    MAIN_THREAD{
        printf("Start benchmarking\n");
    }

    ALL_THREADS{
        ull curr=0;
        ull start=0;
        ull end=0;
        int size=0;
        int type=0;
        while(curr<schedule_size){
            type=schedule[curr];
            curr++;
            size=schedule[curr];
            curr++;
            start=curr;
            while(schedule[curr]!=-1){curr++;}
            end=curr;
            curr++;
            BENCHMARK benchmarks[]={BENCH(bcast),BENCH(gather),BENCH(reduce),BENCH(allreduce),BENCH(scan),BENCH(alltoall)};
            benchmarks[type](world_rank, size, &schedule[start], end-start);
        }
    }
    return 0;

}
int main(int argc, char** argv){
    MPI_Init(NULL, NULL);
    int ret=mpi_main(argc, argv);
    MPI_Finalize();
    return ret;
}
#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "vec.h"
#include "utils.h"

void trace_bench(const char* name, int size, int* nodes, ull node_count){
    MPI_Barrier(MPI_COMM_WORLD);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    if(world_rank==0){
        printf("Benchmarking with strategy \"%s\" (packet size=%d). Communicator=[", name, size);
        for(ull i=0; i<node_count; i++){
            printf("%d%s", nodes[i], (i+1==node_count?"].\n":", "));
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
}

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
#define ALL_THREADS

#define BENCH(x) CONCAT(bench_,x)
#define DEF_BENCH(x) void BENCH(x)(int world_rank, int size, int* nodes, ull node_count)
#define BENCH_DEFAULT do{(void)world_rank; (void)size; (void)nodes; (void)node_count;}while(0)
#define ROOT_THREAD if(rank==0)
#define LEAF_THREADS if(rank!=0)
#define TRACE_BENCH(x) do{trace_bench(STR(x), size, nodes, node_count);}while(0)
DEF_BENCH(bcast){
    MPI_Barrier(MPI_COMM_WORLD);
    TRACE_BENCH(bcast);
    MPI_Comm new_comm;
    int rank;
    if((rank=fork_communicator(world_rank, nodes, node_count, &new_comm))!=-1){
        //printf("%d is %d\n", world_rank, rank);
        char* buffer=malloc((ull)(unsigned int)size);
        ROOT_THREAD {
            for(int i=0; i<size; i++) buffer[i]=(char)hash_i(i);
        }
        ull start_time;
        ull end_time;
        MPI_Barrier(new_comm);
        start_time=get_time();
        MPI_Bcast(buffer, size, MPI_BYTE, 0, new_comm);
        end_time=get_time();
        MPI_Barrier(new_comm);
        ALL_THREADS {
            for(int i=0; i<size; i++) assert(buffer[i]==(char)hash_i(i));
        }
        printf("world rank %d(local rank = %d) = %lf (cost time=%lf)\n", world_rank, rank, bandwidth(1, size, end_time-start_time), ((double)(end_time-start_time))/1000000000);
        free(buffer);
        MPI_Comm_free(&new_comm);
    }
    MPI_Barrier(MPI_COMM_WORLD);
}
DEF_BENCH(gather){
    MPI_Barrier(MPI_COMM_WORLD);
    TRACE_BENCH(gather);
    MPI_Comm new_comm;
    int rank;
    if((rank=fork_communicator(world_rank, nodes, node_count, &new_comm))!=-1){
        //printf("%d is %d\n", world_rank, rank);
        char* send_buffer=malloc((ull)(unsigned int)size);
        char* recv_buffer=0;
        ALL_THREADS {
            for(int i=0; i<size; i++) send_buffer[i]=(char)hash_i(i + rank * 114514);
        }
        MPI_Barrier(new_comm);
        ull start_time;
        ull end_time;
        ROOT_THREAD{
            recv_buffer = malloc((ull)(unsigned int) size * node_count);
            start_time=get_time();
            MPI_Gather(send_buffer, size, MPI_BYTE, recv_buffer, size, MPI_BYTE, 0, new_comm);
            end_time=get_time();

        }
        LEAF_THREADS{
            start_time=get_time();
            MPI_Gather(send_buffer, size, MPI_BYTE, NULL, size, MPI_BYTE, 0, new_comm);
            end_time=get_time();
        }
        
        
        ROOT_THREAD {
            for(int i=0; i<size * (int)node_count; i++) assert(recv_buffer[i]==(char)hash_i((i%size) + 114514 * (i/size)));
        }
        printf("world rank %d(local rank = %d) = %lf (cost time=%lf)\n", world_rank, rank, bandwidth(1, size, end_time-start_time), ((double)(end_time-start_time))/1000000000);
        free(send_buffer);
        free(recv_buffer);
        MPI_Comm_free(&new_comm);
    }
    MPI_Barrier(MPI_COMM_WORLD);
}
DEF_BENCH(reduce){
    MPI_Barrier(MPI_COMM_WORLD);
    TRACE_BENCH(reduce);
    MPI_Comm new_comm;
    int rank;
    if((rank=fork_communicator(world_rank, nodes, node_count, &new_comm))!=-1){
        //printf("%d is %d\n", world_rank, rank);
        char* send_buffer=malloc((ull)(unsigned int)size);
        char* recv_buffer=0;
        ALL_THREADS {
            for(int i=0; i<size; i++) send_buffer[i]=(char)hash_i(i + rank * 114514);
        }
        MPI_Barrier(new_comm);
        ull start_time;
        ull end_time;
        ROOT_THREAD{
            recv_buffer=malloc((ull)(unsigned int)size);


        }

        start_time=get_time();
        MPI_Reduce(send_buffer, recv_buffer, size, MPI_BYTE, MPI_SUM, 0, new_comm);
        end_time=get_time();
        
        
        ROOT_THREAD {
            for(int i=0; i<size; i++){
                char r=0;
                for(ull j=0; j<node_count; j++){
                    r=(char)(r+((char)hash_i((i) + 114514 * ((int)j))));
                }
                assert(recv_buffer[i]==r);
            }
        }
        printf("world rank %d(local rank = %d) = %lf (cost time=%lf)\n", world_rank, rank, bandwidth(1, size, end_time-start_time), ((double)(end_time-start_time))/1000000000);
        free(send_buffer);
        free(recv_buffer);
        MPI_Comm_free(&new_comm);
    }
    MPI_Barrier(MPI_COMM_WORLD);
}
DEF_BENCH(allreduce){
    MPI_Barrier(MPI_COMM_WORLD);
    TRACE_BENCH(allreduce);
    MPI_Comm new_comm;
    int rank;
    if((rank=fork_communicator(world_rank, nodes, node_count, &new_comm))!=-1){
        //printf("%d is %d\n", world_rank, rank);
        char* send_buffer=malloc((ull)(unsigned int)size);
        char* recv_buffer=0;
        ALL_THREADS {
            for(int i=0; i<size; i++) send_buffer[i]=(char)hash_i(i + rank * 114514);
        }
        MPI_Barrier(new_comm);
        ull start_time;
        ull end_time;
        recv_buffer=malloc((ull)(unsigned int)size);

        start_time=get_time();
        MPI_Allreduce(send_buffer, recv_buffer, size, MPI_BYTE, MPI_SUM, new_comm);
        end_time=get_time();
        
        
        ALL_THREADS {
            for(int i=0; i<size; i++){
                char r=0;
                for(ull j=0; j<node_count; j++){
                    r=(char)(r+((char)hash_i((i) + 114514 * ((int)j))));
                }
                assert(recv_buffer[i]==r);
            }
        }
        printf("world rank %d(local rank = %d) = %lf (cost time=%lf)\n", world_rank, rank, bandwidth(1, size, end_time-start_time), ((double)(end_time-start_time))/1000000000);
        free(send_buffer);
        free(recv_buffer);
        MPI_Comm_free(&new_comm);
    }
    MPI_Barrier(MPI_COMM_WORLD);
}
DEF_BENCH(scan){
    MPI_Barrier(MPI_COMM_WORLD);
    TRACE_BENCH(scan);
    MPI_Comm new_comm;
    int rank;
    if((rank=fork_communicator(world_rank, nodes, node_count, &new_comm))!=-1){
        //printf("%d is %d\n", world_rank, rank);
        char* send_buffer=malloc((ull)(unsigned int)size);
        char* recv_buffer=0;
        ALL_THREADS {
            for(int i=0; i<size; i++) send_buffer[i]=(char)hash_i(i + rank * 114514);
        }
        MPI_Barrier(new_comm);
        ull start_time;
        ull end_time;
        recv_buffer=malloc((ull)(unsigned int)size);

        start_time=get_time();
        MPI_Scan(send_buffer, recv_buffer, size, MPI_BYTE, MPI_SUM, new_comm);
        end_time=get_time();
        
        
        ALL_THREADS {
            for(int i=0; i<size; i++){
                char r=0;
                for(ull j=0; j<=(ull)rank; j++){
                    r=(char)(r+((char)hash_i((i) + 114514 * ((int)j))));
                }
                assert(recv_buffer[i]==r);
            }
        }
        printf("world rank %d(local rank = %d) = %lf (cost time=%lf)\n", world_rank, rank, bandwidth(1, size, end_time-start_time), ((double)(end_time-start_time))/1000000000);
        free(send_buffer);
        free(recv_buffer);
        MPI_Comm_free(&new_comm);
    }
    MPI_Barrier(MPI_COMM_WORLD);
}
DEF_BENCH(alltoall){
    MPI_Barrier(MPI_COMM_WORLD);
    TRACE_BENCH(alltoall);
    MPI_Comm new_comm;
    int rank;
    if((rank=fork_communicator(world_rank, nodes, node_count, &new_comm))!=-1){
        //printf("%d is %d\n", world_rank, rank);
        char* send_buffer=malloc((ull)(unsigned int)size * node_count);
        char* recv_buffer=0;
        ALL_THREADS {
            for(int i=0; i<(int)size * (int)node_count; i++) send_buffer[i]=(char)hash_i(i + rank * 114514);
        }
        MPI_Barrier(new_comm);
        ull start_time;
        ull end_time;
        recv_buffer=malloc((ull)(unsigned int)size * node_count);

        start_time=get_time();
        MPI_Alltoall(send_buffer, size, MPI_BYTE, recv_buffer, size, MPI_BYTE, new_comm);
        end_time=get_time();
        
        
        ALL_THREADS {
            for(int i=0; i<size * (int)node_count; i++) assert(recv_buffer[i]==(char)hash_i(((i%size) + rank * size) + 114514 * (i/size)));
        }
        printf("world rank %d(local rank = %d) = %lf (cost time=%lf)\n", world_rank, rank, bandwidth(1, size, end_time-start_time), ((double)(end_time-start_time))/1000000000);
        free(send_buffer);
        free(recv_buffer);
        MPI_Comm_free(&new_comm);
    }
    MPI_Barrier(MPI_COMM_WORLD);
}
#define MAIN_THREAD if(world_rank==0)
#define SUB_THREADS if(world_rank!=0)

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
    free(schedule);
    return 0;

}
int main(int argc, char** argv){
    MPI_Init(NULL, NULL);
    int ret=mpi_main(argc, argv);
    MPI_Finalize();
    return ret;
}
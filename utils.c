#include <time.h>
#include "utils.h"
unsigned int hash(unsigned int x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

int hash_i(int x){
    return (int)hash((unsigned int)x);
}

unsigned long long get_time(){
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    unsigned long long t_s,t_ns;
    t_s=(unsigned long long)t.tv_sec;
    t_ns=(unsigned long long)t.tv_nsec;
    return t_s*1000000000ULL+t_ns;
}

double bandwidth(int send_cnt, int send_size, ull cost_time){
    return (double)(send_cnt)*(double)(send_size) / (((double)cost_time)/1000);
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

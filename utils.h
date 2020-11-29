#ifndef _UTILS_H
#define _UTILS_H
#define _CONCAT(a,b) a##b
#define CONCAT(a,b) _CONCAT(a,b)
#define _STR(x) #x
#define STR(x) _STR(x)
typedef unsigned long long ull;
unsigned int hash(unsigned int x);
int hash_i(int x);
unsigned long long get_time();
double bandwidth(int send_cnt, int send_size, ull cost_time);
#include <mpi.h>
int fork_communicator(int world_rank, int* nodes, ull node_count, MPI_Comm* new_comm);
#endif
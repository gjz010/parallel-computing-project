#ifndef _UTILS_H
#define _UTILS_H
#define _CONCAT(a,b) a##b
#define CONCAT(a,b) _CONCAT(a,b)
#define _STR(x) #x
#define STR(x) _STR(x)
#include <mpi.h>
char * my_strtok_r (char *s, const char *delim, char **save_ptr);
typedef unsigned long long ull;
typedef long long ll;
unsigned int hash(unsigned int x);
int hash_i(int x);
unsigned long long get_time();
double bandwidth(int send_cnt, int send_size, ull cost_time);
int fork_communicator(int world_rank, int* nodes, ull node_count, MPI_Comm* new_comm);
#endif

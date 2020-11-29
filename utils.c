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
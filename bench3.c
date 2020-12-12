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

void single_write(MPI_File fh, int offset, int size){
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    char* buffer = malloc((unsigned long)size);
    for(int i=0; i<size; i++){
        buffer[i]=(char)hash_i(offset+i);
    }
    unsigned long long start_time=get_time();
    MPI_File_write_at(fh, offset, buffer, size, MPI_BYTE, MPI_STATUS_IGNORE);
    unsigned long long end_time=get_time();
    printf("W world rank %d (offset = %d, size = %d) = %lf (cost time=%lf)\n", world_rank, offset, size, bandwidth(1, size, end_time-start_time), ((double)(end_time-start_time))/1000000000);
    free(buffer);
}
void single_read(MPI_File fh, int offset, int size){
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    
    char* buffer = malloc((unsigned long)size);
    unsigned long long start_time=get_time();
    MPI_File_read_at(fh, offset, buffer, size, MPI_BYTE, MPI_STATUS_IGNORE);
    unsigned long long end_time=get_time();
    //for(int i=0; i<sizes[world_rank]; i++){
    //    assert(buffer[i] == (char)hash(world_rank * 114514 + i));
    //}
    printf("R world rank %d (offset = %d, size = %d) = %lf (cost time=%lf)\n", world_rank, offset, size, bandwidth(1, size, end_time-start_time), ((double)(end_time-start_time))/1000000000);
    free(buffer);
}

void collective_read(MPI_File fh, int* sizes, MPI_Comm comm){
    int world_size;
    MPI_Comm_size(comm, &world_size);
    int world_rank;
    MPI_Comm_rank(comm, &world_rank);
    int offset=0;
    for(int i=0; i<world_rank; i++){
        offset+=sizes[i];
    }
    char* buffer = malloc((unsigned long)sizes[world_rank]);
    
    MPI_Barrier(comm);
    unsigned long long start_time=get_time();
    MPI_File_read_at_all(fh, offset, buffer, sizes[world_rank], MPI_BYTE, MPI_STATUS_IGNORE);
    unsigned long long end_time=get_time();
    for(int i=0; i<sizes[world_rank]; i++){
        assert(buffer[i] == (char)hash_i(world_rank * 114514 + i));
    }
    int real_world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &real_world_rank);
    printf("CR world rank %d(local rank = %d, offset = %d, size = %d) = %lf (cost time=%lf)\n", real_world_rank, world_rank, offset, sizes[world_rank], bandwidth(1, sizes[world_rank], end_time-start_time), ((double)(end_time-start_time))/1000000000);
    free(buffer);
    MPI_Barrier(comm);
}
void collective_write(MPI_File fh, int* sizes, MPI_Comm comm){
    int world_size;
    MPI_Comm_size(comm, &world_size);
    int world_rank;
    MPI_Comm_rank(comm, &world_rank);
    int offset=0;
    for(int i=0; i<world_rank; i++){
        offset+=sizes[i];
    }
    char* buffer = malloc((unsigned long)sizes[world_rank]);
    for(int i=0; i<sizes[world_rank]; i++){
        buffer[i] = (char)hash_i(world_rank * 114514 + i);
    }
    MPI_Barrier(comm);
    unsigned long long start_time=get_time();
    MPI_File_write_at_all(fh, offset, buffer, sizes[world_rank], MPI_BYTE, MPI_STATUS_IGNORE);
    unsigned long long end_time=get_time();
    int real_world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &real_world_rank);
    printf("CW world rank %d(local rank = %d, offset = %d, size = %d) = %lf (cost time=%lf)\n", real_world_rank, world_rank, offset, sizes[world_rank], bandwidth(1, sizes[world_rank], end_time-start_time), ((double)(end_time-start_time))/1000000000);
    free(buffer);
    MPI_Barrier(comm);
}



int mpi_main(int argc, char** argv){
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    if(argc<=2){
        printf("MPI Bandwidth Tester #2\nusage: %s buffer_path task1 task2 task3 ...\ntask is defined by: op1,n1,s1:op2,n2,s2:... or c,n1,s1,n2,s2,...\n", argv[0]);
        return 1;
    }
    const char* path=argv[1];
    vec_int v = NEW_VEC;
    for(int i=2; i<argc; i++){
        char* arg=argv[i];
        if(arg[0]=='c'){ // Collective benchmarking
            assert(arg[1]==',');
            char* s1;
            char* s_group = my_strtok_r(arg+2, ",", &s1);
            vec_push(&v, 1);
            while(s_group){
                int index;
                assert(sscanf(s_group, "%d", &index)==1);
                s_group = my_strtok_r(NULL, ",", &s1);
                assert(s_group);
                int size;
                assert(sscanf(s_group, "%d", &size)==1);
                s_group = my_strtok_r(NULL, ",", &s1);
                vec_push(&v, index);
                vec_push(&v, size);
            }
            vec_push(&v, -1);
        }else{
            char* s1;
            char* s_group = my_strtok_r(arg, ":", &s1);
            vec_push(&v, 0);
            while(s_group){
                char cop;
                int index, size;
                assert(sscanf(s_group, "%c,%d,%d", &cop, &index, &size)==3);
                assert(size>0);
                vec_push(&v, index);
                if(cop=='w'){
                    vec_push(&v, size);
                }else if(cop=='r'){
                    vec_push(&v, -size);
                }else{
                    assert(0);
                }
                s_group = my_strtok_r(NULL, ":", &s1);
            }
            vec_push(&v, -1);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_File fh;
    MPI_File_open(MPI_COMM_WORLD, path, MPI_MODE_CREATE|MPI_MODE_RDWR, MPI_INFO_NULL, &fh); 
    MAIN_THREAD {
        printf("Opened.\n");
        for(int i=0; i<(int)v.size; i++){
            printf("%d ", v.data[i]);
        }
        printf("\n");
    }
    unsigned long long index=0;
    while(index < v.size){
        if(v.data[index]==0){
            index++;
            // Noncollective ops
            int offset=0;
            int size=0;
            int total_size=0;
            ull new_index=index;
            while(v.data[new_index]!=-1){
                int curr_size=v.data[new_index+1];
                total_size+=curr_size;
                new_index+=2;
            }
            while(v.data[index]!=-1){
                int curr_index = v.data[index];
                int curr_size = v.data[index+1];
                if(curr_index == world_rank){
                    assert(size==0);
                    size = curr_size;
                    while(v.data[index]!=-1) index+=2;
                    break;
                }else{
                    offset += (curr_size>0?curr_size:-curr_size);
                }
                index+=2;
            }
            index++;
            MPI_Barrier(MPI_COMM_WORLD);
            ull start_time=0;
            ull end_time=0;
            MAIN_THREAD {start_time=get_time();}
            if(size!=0){
                if(size>0){
                    single_write(fh, offset, size);
                }else if(size<0){
                    single_read(fh, offset, -size);
                }
            }
            MPI_Barrier(MPI_COMM_WORLD);
            MAIN_THREAD {end_time=get_time(); printf("Mixed total bandwidth = %lf\n", bandwidth(1, (int)total_size, end_time-start_time));}
        }else if(v.data[index]==1){
            index++;
            // Collective ops
            int order[world_size];
            int sizes[world_size];
            unsigned long long total_size=0;
            for(int i=0; i<world_size; i++){
                order[i]=-1;
                sizes[i]=-1;
            }
            int t=0;
            while(v.data[index]!=-1){
                int wr=v.data[index];
                int size=v.data[index+1];
                order[t]=wr;
                sizes[t]=size;
                total_size+=(ull)size;
                t++;
                index+=2;
            }
            index++;
            MPI_Comm new_comm;
            int rank=fork_communicator(world_rank, order, (ull)t, &new_comm);
            MPI_Barrier(MPI_COMM_WORLD);
            ull start_time=0;
            ull end_time=0;
#define FIRST_THREAD if(rank==order[0])
            if(rank!=-1){
                FIRST_THREAD {start_time=get_time();}
                collective_write(fh, sizes, new_comm);
                FIRST_THREAD {end_time=get_time(); printf("CW total bandwidth = %lf\n", bandwidth(1, (int)total_size, end_time-start_time));}
                MPI_Barrier(new_comm);
                FIRST_THREAD {start_time=get_time();}
                collective_read(fh, sizes, new_comm);
                FIRST_THREAD {end_time=get_time(); printf("CR total bandwidth = %lf\n", bandwidth(1, (int)total_size, end_time-start_time));}
            }
            MPI_Barrier(MPI_COMM_WORLD);
            MPI_Comm_free(&new_comm);
            
        }else{
            assert(0);
        }
    }
    MPI_File_close(&fh);
    return 0;
}
int main(int argc, char** argv){
    MPI_Init(NULL, NULL);
    int ret=mpi_main(argc, argv);
    if(ret!=0){
        return ret;
    }
    MPI_Finalize();
    return ret;
}

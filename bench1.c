#include <ctype.h>
#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "vec.h"
#include "utils.h"
#include <assert.h>

typedef struct configuration{
    int number_nodes;
    int send_cnt;
    int send_size;
    int max_groups;
    int* grouping;
} config;

int get_config(int number_nodes, int argc, char** argv, config** output){
    int c;
    config* cfg=malloc(sizeof(*cfg));
    int err=0;
    cfg->number_nodes=number_nodes;
    cfg->send_cnt=-1;
    cfg->send_size=-1;
    cfg->max_groups=-1;
    cfg->grouping=NULL;
    #define READ_FIELD(x) \
    if(x!=-1){\
        goto dup_argument;\
    }\
    if(!optarg || sscanf(optarg, "%d", &x)!=1 || x<=0){\
        goto bad_argument;\
    }
    while((c=getopt(argc, argv, "c:s:g:h"))!=-1){
        if(c=='c'){
            READ_FIELD(cfg->send_cnt);
        }else if(c=='s'){
            READ_FIELD(cfg->send_size);
        }else if(c=='g'){
            if(cfg->max_groups!=-1){
                goto dup_argument;
            }
            if(!optarg){
                goto bad_argument;
            }
            if(cfg->number_nodes==-1){
                fprintf(stderr, "`-%c' need to be placed after `-n'!\n", c);
                err=3;
                goto error;
            }
            cfg->grouping = malloc(sizeof(int) * (unsigned int) cfg->number_nodes);
            int current_group=0;
            int counted_nodes=0;
            for(int i=0; i<cfg->number_nodes; i++){
                cfg->grouping[i]=-1;
            }
            char* s1, *s2;
            char* s_group = strtok_r(optarg, ":", &s1);
            while(s_group){
                char* s_elm = strtok_r(s_group, ",", &s2);
                while(s_elm){
                    int index;
                    if(sscanf(s_elm, "%d", &index)!=1){
                        goto bad_grouping;
                    }
                    if(index>=cfg->number_nodes){
                        fprintf(stderr, "Bad index %d! \n", index);
                        err=4;
                        goto error;
                    }
                    if(cfg->grouping[index]!=-1){
                        fprintf(stderr, "Index %d appeared twice in different groups %d and %d!\n", index, cfg->grouping[index], current_group);
                        err=5;
                        goto error;
                    }
                    cfg->grouping[index]=current_group;
                    counted_nodes++;
                    s_elm = strtok_r(NULL, ",", &s2);
                }
                current_group++;
                s_group=strtok_r(NULL, ":", &s1);
            }
            if(counted_nodes!=cfg->number_nodes){
                fprintf(stderr, "Not all nodes are put into groups! Expected %d, found %d.\n", cfg->number_nodes, counted_nodes);
                err=6;
                goto error;
            }
            cfg->max_groups=current_group;
        }else if(c=='h'){
            fprintf(stderr, "MPI Bandwidth Tester #1\nusage: %s -n [node count] -c [send count] -s [send size] <-g [member1,member2;group2]>\n", argv[0]);
            err=0;
            goto error;
            return 0;
        }else if(c=='?'){
            fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            err=1;
            goto error;
        }
    }

    if(cfg->number_nodes==-1 || cfg->send_cnt==-1 || cfg->send_size==-1){
        fprintf(stderr, "MPI Bandwidth Tester #1\nusage: %s -n [node count] -c [send count] -s [send size] <-g [member1,member2;group2]>\n", argv[0]);
        err=7;
        goto error;
    }

    // Fall back to one-group case for convenience.
    if(cfg->max_groups==-1){
        cfg->grouping = malloc(sizeof(int) * (unsigned int) cfg->number_nodes);
        for(int i=0; i<cfg->number_nodes; i++){
            cfg->grouping[i]=0;
        }
        cfg->max_groups=0;
    }
    *output=cfg;
    return 0;

error:
    free(cfg->grouping);
    free(cfg);
    return err;

bad_grouping:
    fprintf(stderr, "Error while parsing groups.\n");
    err=4;
    goto error;
bad_argument:
    fprintf(stderr, "Need argument: `-%c'\n", c);
    err=2;
    goto error;
dup_argument:
    fprintf(stderr, "Duplicated argument: `-%c'\n", c);
    err=2;
    goto error;
}

void destroy_config(config* cfg){
    free(cfg->grouping);
    free(cfg);
}

// find the best parallel scheudle by greedy algorithm.
// The result is encoded in the following way:
// 1. For every tick there are pairs of ints indicating the peer.
// 2. Each ``tick'' is ended by an end mark of -1.
// 3. In-group pairs are scheduled first, then across-group pairs.
// The schedule table is finally send to all nodes so that they can start scheduling.
int find_optimal_schedule(config* cfg, int** result){
    vec_int schedule = NEW_VEC;

    vec_int* collections = calloc(sizeof(vec_int), (size_t)cfg->max_groups);
    for(int i=0; i<cfg->number_nodes; i++){
        vec_push(&collections[cfg->grouping[i]],i);
    }
    vec_int* edges = calloc(sizeof(vec_int), (size_t)cfg->max_groups);
    for(int i=0; i<cfg->max_groups; i++){
        for(size_t j=0; j<collections[i].size; j++){
            for(size_t k=j+1; k<collections[i].size; k++){
                vec_push(&edges[i], collections[i].data[j]);
                vec_push(&edges[i], collections[i].data[k]);
                vec_push(&edges[i], collections[i].data[k]);
                vec_push(&edges[i], collections[i].data[j]);
            }
        }
    }
    size_t offset=0;
    while(1){
        int flag=0;
        for(int i=0; i<cfg->max_groups; i++){
            if(edges[i].size>offset+1){
                flag=1;
                vec_push(&schedule, edges[i].data[offset]);
                vec_push(&schedule, edges[i].data[offset+1]);
            }
        }
        offset=offset+2;
        
        if(flag){
            vec_push(&schedule, -1);
        }else{
            break;
        }
    }
    for(int i=0; i<cfg->max_groups; i++){
        vec_destroy(&collections[i]);
        vec_destroy(&edges[i]);
    }
    free(collections);
    free(edges);
    // schedule across-pairs.
    for(int i=0; i<cfg->number_nodes; i++){
        for(int j=i+1; j<cfg->number_nodes; j++){
            if(cfg->grouping[i]!=cfg->grouping[j]){
                vec_push(&schedule, i);
                vec_push(&schedule, j);
                vec_push(&schedule, -1);
                vec_push(&schedule, j);
                vec_push(&schedule, i);
                vec_push(&schedule, -1);
                printf("Across pair (%d,%d) detected.\n", i, j);
            }
        }
    }
    *result=schedule.data;
    return (int)schedule.size;
}


#define MAIN_THREAD if(world_rank==0)
#define SUB_THREADS if(world_rank!=0)
#define ALL_THREADS

int mpi_main(int argc, char** argv){
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    
    config* cfg=NULL;
    int* schedule_table=NULL;
    int table_size=0;
    int real_table_count;
    int error=0;
    MAIN_THREAD {
        error=get_config(world_size, argc, argv, &cfg);
        if(!cfg){
            goto bcast;
        }
        
        table_size=find_optimal_schedule(cfg, &schedule_table);
        printf("Optimal Schedule: %d\n", table_size);
        int index=0;
        while(index<table_size){
            if(schedule_table[index]==-1){
                putchar('\n');
                index+=1;
            }else{
                printf("(%d,%d) ", schedule_table[index], schedule_table[index+1]);
                index+=2;
            }
            
        }
    }
bcast:
    real_table_count=table_size;
    MPI_Bcast(&real_table_count,1,MPI_INT,0,MPI_COMM_WORLD);
    if(real_table_count==0){
        MAIN_THREAD{
            if(error==0){
                printf("Schedule table size=0! Exiting...\n");
            }else{
                printf("Error code = %d\n", error);
            }
            return error;
        }
        printf("Main thread exited abnormally. Exiting...\n");
        return 0;
    }
    SUB_THREADS{
        schedule_table=malloc(sizeof(int)*((size_t)real_table_count));
    }
    MPI_Bcast(schedule_table, real_table_count, MPI_INT, 0, MPI_COMM_WORLD);
    printf("Thread %d: %d\n", world_rank, real_table_count);
    MPI_Barrier(MPI_COMM_WORLD);
    int send_size=0;
    int send_cnt=0;
    

    MAIN_THREAD{
        send_size=cfg->send_size;
        send_cnt=cfg->send_cnt;
    }


    ull* perf_vec = calloc(sizeof(ull), 2*(size_t)world_size);
    ALL_THREADS {
        MPI_Bcast(&send_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&send_cnt, 1, MPI_INT, 0, MPI_COMM_WORLD);
        char* send_data = malloc(sizeof(char) * (size_t) send_size);
        char* recv_data = malloc(sizeof(char) * (size_t) send_size);
        for(int i=0; i<send_size; i++){
            send_data[i]=(char)hash_i(i);
        }
        // perform all steps.
        int index=0;
        int max_ticks = 0;
        for(int i=0; i<real_table_count; i++){
            if(schedule_table[i]==-1){max_ticks++;}
        }
        for(int i=0; i<max_ticks; i++){
            MPI_Barrier(MPI_COMM_WORLD);
            MAIN_THREAD{
                printf("Tick %d: ", i);
                int index_tmp = index;
                while(index_tmp<real_table_count){
                    if(schedule_table[index_tmp]==-1){
                        break;
                    }
                    printf("(%d, %d) ", schedule_table[index_tmp], schedule_table[index_tmp+1]);
                    index_tmp+=2;
                }
                printf("\n");
            }
            int current_from=-1;
            int current_to=-1;
            //printf("Proc %d\n", world_rank);
            MPI_Barrier(MPI_COMM_WORLD);
            //printf("Proc %d\n", world_rank);
            while(index<real_table_count){
                if(schedule_table[index]==-1){
                    // Do nothing.
                    index++;
                    break;
                }
                if(schedule_table[index]==world_rank){
                    assert(current_from==-1);
                    assert(current_to==-1);
                    current_to=schedule_table[index+1];
                    
                }else if(schedule_table[index+1]==world_rank){
                    assert(current_from==-1);
                    assert(current_to==-1);
                    current_from=schedule_table[index];
                    //index+=2;
                }
                index+=2;

            }
            //printf("Proc %d\n", world_rank);
            MPI_Barrier(MPI_COMM_WORLD);
#define SENDING_THREAD if(current_to!=-1)
#define RECEIVING_THREAD if(current_from!=-1)
#define IDLE_THREAD if(current_from==-1 && current_to==-1)
//printf("Proc %d\n", world_rank);
            ull start_tick = 0;
            ull end_tick = 0;
            SENDING_THREAD{
                start_tick=get_time();
                for(int i=0; i<send_cnt; i++)
                    MPI_Send(send_data, send_size, MPI_BYTE, current_to, i, MPI_COMM_WORLD);
                end_tick=get_time();
            }
            RECEIVING_THREAD{
                start_tick=get_time();
                for(int i=0; i<send_cnt; i++)
                    MPI_Recv(recv_data, send_size, MPI_BYTE, current_from, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                end_tick=get_time();
            }
            //printf("Proc %d\n", world_rank);
            MPI_Barrier(MPI_COMM_WORLD);
            SENDING_THREAD{
                //printf("%lld %lld %d\n", start_tick, end_tick, end_tick>=start_tick);
                assert(end_tick>=start_tick);
                perf_vec[current_to]=end_tick-start_tick;
                //printf("Send time (%d->%d): %lld\n", world_rank, current_to, end_tick-start_tick);
            }
            RECEIVING_THREAD{
                assert(end_tick>=start_tick);
                perf_vec[current_from + world_size]=end_tick-start_tick;
                //printf("Recv time (%d->%d): %lld\n", current_from, world_rank, end_tick-start_tick);
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }
        
        // write the result to binary by mpiio.
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MAIN_THREAD { printf("Writing output binary file!\n"); }
    MPI_File fh;
    ALL_THREADS {
        
        MPI_File_open(MPI_COMM_WORLD, "bench1.out", MPI_MODE_CREATE|MPI_MODE_RDWR, MPI_INFO_NULL, &fh); 
        //MPI_Datatype filetype_vec;
        //MPI_Type_contiguous(world_size*2, MPI_LONG_LONG_INT, &filetype_vec);
        MPI_File_set_view(fh, (long long)(2*(ull)world_rank * (ull)world_size * sizeof(ull)), MPI_LONG_LONG_INT, MPI_LONG_LONG_INT, "native", MPI_INFO_NULL);
        MPI_File_write_all(fh, perf_vec, 2*world_size, MPI_LONG_LONG_INT, MPI_STATUS_IGNORE);
        MPI_File_set_view(fh, 0, MPI_LONG_LONG_INT, MPI_LONG_LONG_INT, "native", MPI_INFO_NULL);
    }
    free(perf_vec);
    MPI_Barrier(MPI_COMM_WORLD);
    MAIN_THREAD {
        
        // main thread read from mpiio and print the output.
        ull* perf_mat = malloc(2*(size_t)world_size*(size_t)world_size*sizeof(ull));
        MPI_File_read_at(fh, 0, perf_mat, world_size * world_size * 2, MPI_LONG_LONG_INT, MPI_STATUS_IGNORE);
        printf("Send Matrix (MBps):\n");
        double total_size = send_size * send_cnt;
        for(int i=0; i<world_size; i++){
            for(int j=0; j<world_size; j++){
                if(i==j) {printf("-\t");continue;}
                double bandwidth =total_size/(((double)perf_mat[2*world_size*i+j] )/1000);
                printf("%lf\t", bandwidth);
            }
            putchar('\n');
        }
        printf("Recv Matrix (MBps):\n");
        for(int i=0; i<world_size; i++){
            for(int j=0; j<world_size; j++){
                if(i==j) {printf("-\t");continue;}
                double bandwidth =total_size/(((double)perf_mat[2*world_size*i+j+world_size] )/1000);
                printf("%lf\t", bandwidth);
            }
            putchar('\n');
        }
        
    }
    MPI_File_close(&fh);
    return 0;

}

int main(int argc, char** argv){
    MPI_Init(NULL, NULL);
    int ret=mpi_main(argc, argv);
    MPI_Finalize();
    return ret;
}
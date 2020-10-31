#include <ctype.h>
#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct configuration{
    int number_nodes;
    int send_cnt;
    int send_size;
    int max_groups;
    int* grouping;
} config;

int get_config(int argc, char** argv, config** output){
    int c;
    config* cfg=malloc(sizeof(*cfg));
    int err=0;
    cfg->number_nodes=-1;
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
    while((c=getopt(argc, argv, "n:c:s:g:h"))!=-1){
        if(c=='n'){
            READ_FIELD(cfg->number_nodes);
        }else if(c=='c'){
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
            char* s_group = strtok_r(optarg, ";", &s1);
            while(s_group){
                char* s_elm = strtok_r(s_group, ",", &s2);
                while(s_elm){
                    int index;
                    if(sscanf(s_elm, "%d", &index)!=1){
                        goto bad_grouping;
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
                s_group=strtok_r(NULL, ";", &s1);
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
    int* same_in_group=calloc(sizeof(int) * (size_t)cfg->max_groups);
    for(int i=0; i<cfg->number_nodes; i++){
        same_in_group[cfg->grouping]
    }

    for(int i=0; i<cfg->number_nodes; i++){
        for(int j=i+1; j<cfg->number_nodes; j++){
            if(cfg->grouping[i]!=cfg->grouping[j]){
                vec_push(&schedule, i);
                vec_push(&schedule, j);
                vec_push(&schedule, -1);
            }
        }
    }
    *result=schedule->data;
    return schedule->size;
}
#define MAIN_THREAD if(world_rank==0)
#define SUB_THREADS if(world_rank!=0)
#define ALL_THREADS
int main(int argc, char** argv){
    MPI_Init(NULL, NULL);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    
    config* cfg;
    int* schedule_table;
    int table_size=0;
    MAIN_THREAD {
        int ret=get_config(argc, argv, &cfg);
        if(!cfg){
            return ret;
        }
        
        table_size=find_optimal_schedule(cfg, &schedule_table);
        // broadcast the schedule table to all nodes.

    }
    (void)table_size;
    SUB_THREADS {
        // receive the scheduling table

    }
    MPI_Barrier(MPI_COMM_WORLD);
    ALL_THREADS {
        // perform all steps.

        // write the result to binary by mpiio.
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MAIN_THREAD {
        // main thread read from mpiio and print the output.
    }
    MPI_Finalize();
}
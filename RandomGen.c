#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv){
    srand((unsigned int)time(0));
    FILE* f=fopen("matrix", "w+");
    if(argc<=1){
        fprintf(stderr, "usage: %s [row] [column]\n", argv[0]);
        return 1;
    }
    int M,N;
    if(sscanf(argv[1], "%d", &M)!=1){
        fprintf(stderr, "usage: %s [row] [column]\n", argv[0]);
        return 1;
    }
    if(sscanf(argv[2], "%d", &N)!=1){
        fprintf(stderr, "usage: %s [row] [column]\n", argv[0]);
        return 1;
    }

    for(int i=0; i<M; i++){
        for(int j=0; j<N; j++){
            fprintf(f, "%lf\t", (double)(rand())/RAND_MAX);
        }
        fprintf(f, "\n");
    }
    fclose(f);
}
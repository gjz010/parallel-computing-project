#include <assert.h>
#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include "utils.h"
#define MAIN_THREAD if(world_rank==0)
#define SUB_THREADS if(world_rank!=0)
#define ALL_THREADS

  
int main(int argc, char** argv){
    MPI_Init(NULL, NULL);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    int M,N,Nchunk,Npadded;
    double* ans=NULL;
    double* pieces=NULL;
    double* matrix=NULL;
    MAIN_THREAD{

    
        assert(argc==3);
        assert(sscanf(argv[1], "%d", &M)==1);
        assert(sscanf(argv[2], "%d", &N)==1);
        
        FILE* f=fopen("matrix", "r");
        assert(f);
        Nchunk = (N+world_size-1)/world_size;
        Npadded = Nchunk * world_size;
        printf("M=%d N=%d world_size=%d Nchunk=%d Npadded=%d\n", M, N, world_size, Nchunk, Npadded);
        matrix = calloc((size_t)(M * Npadded), sizeof(double));
        for(int i=0; i<M; i++){
            for(int j=0; j<N; j++){
                assert(fscanf(f, "%lf", &matrix[i*Npadded+j])==1);
            }
        }
        fclose(f);
    }
    printf("IO done\n");
    ull end_time=0, start_time=0;
    MAIN_THREAD{
        start_time=get_time();
    }
    // Files are chunked by columns.
    ALL_THREADS {
        MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&Nchunk, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&Npadded, 1, MPI_INT, 0, MPI_COMM_WORLD);
        ans = calloc((size_t)(M*M+M), sizeof(double));
        pieces = malloc(sizeof(double)*(size_t)M*(size_t)Nchunk);
    }
    
    MAIN_THREAD {
        for(int i=0; i<M; i++){
            MPI_Scatter(&matrix[i*Npadded], Nchunk, MPI_DOUBLE, &pieces[i*Nchunk], Nchunk, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        }
    }
    SUB_THREADS {
        for(int i=0; i<M; i++){
            MPI_Scatter(NULL, Nchunk, MPI_DOUBLE, &pieces[i*Nchunk], Nchunk, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        }
    }
    printf("Start Computation rank=%d M=%d Nchunk=%d\n", world_rank, M, Nchunk);
    ALL_THREADS {
        for(int j=0; j<M; j++){
            for(int k=0; k<Nchunk; k++){
                ans[M*M+j] += pieces[j*Nchunk + k] * pieces[j*Nchunk + k];
            }
        }
        for(int i=0; i<M; i++){
            for(int j=0; j<M; j++){
                for(int k=0; k<Nchunk; k++){
                    ans[i*M+j] += pieces[i*Nchunk + k] * pieces[j*Nchunk + k];
                }
            }
        }
    }
    printf("Finished rank=%d\n", world_rank);
    SUB_THREADS {
        MPI_Reduce(ans, NULL, M*M+M, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    }
    
    MAIN_THREAD{
        MPI_Reduce(MPI_IN_PLACE, ans, M*M+M, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        end_time=get_time();
        printf("Computation finished in %lf ms.\n", (double)(end_time-start_time)/1000000.0);
        FILE* f_alpha=fopen("AlphasMPI.mat", "w");
        FILE* f_beta=fopen("BetasMPI.mat", "w");
        FILE* f_gamma=fopen("GammasMPI.mat", "w");
        //fprintf(f_alpha, "%d %d\n", M, N);
        printf("Writing alpha\n");
        for(int i=0; i<M; i++){
            for(int j=0; j<N; j++){
                fprintf(f_alpha, "%.3lf\t", ans[M*M+i]);
            }
            fprintf(f_alpha, "\n");
        }
        printf("Writing beta\n");
        for(int i=0; i<M; i++){
            for(int j=0; j<N; j++){
                fprintf(f_beta, "%.3lf\t", ans[M*M+j]);
            }
            fprintf(f_beta, "\n");
        }
        printf("Writing gamma\n");
        for(int i=0; i<M; i++){
            for(int j=0; j<N; j++){
                fprintf(f_gamma, "%.3lf\t", ans[i*M+j]);
            }
            fprintf(f_gamma, "\n");
        }
        fclose(f_alpha);
        fclose(f_beta);
        fclose(f_gamma);
    }

    
    MPI_Finalize();
    return 0;
}
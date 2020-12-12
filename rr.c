#include <assert.h>
#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include "utils.h"
#define MAIN_THREAD if(world_rank==0)
#define SUB_THREADS if(world_rank!=0)
#define ALL_THREADS

void read_matrix(const char* path, int M, int N, int Npadded, double* mat);
void write_matrix(const char* path, int M, const double* mat);
void write_matrix_i(const char* path, int M, const double* mat);
void write_matrix_j(const char* path, int M, const double* mat);

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
        
        //FILE* f=fopen("matrix", "r");
        //assert(f);
        Nchunk = (N+world_size-1)/world_size;
        Npadded = Nchunk * world_size;
        printf("M=%d N=%d world_size=%d Nchunk=%d Npadded=%d\n", M, N, world_size, Nchunk, Npadded);
        matrix = calloc((size_t)(M * Npadded), sizeof(double));
	assert(matrix);
        read_matrix("matrix", M, N, Npadded, matrix);
        //fclose(f);
        printf("IO done\n");
    }
    MPI_Barrier(MPI_COMM_WORLD);
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
	assert(ans);
        pieces = malloc(sizeof(double)*(size_t)M*(size_t)Nchunk);
	assert(pieces);
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
        MPI_Barrier(MPI_COMM_WORLD);

    }
    
    MAIN_THREAD{
        MPI_Reduce(MPI_IN_PLACE, ans, M*M+M, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        end_time=get_time();
        printf("Computation finished in %lf ms.\n", (double)(end_time-start_time)/1000000.0);
        //fprintf(f_alpha, "%d %d\n", M, N);
        printf("Writing alpha\n");
        write_matrix_i("AlphasMPI.mat", M, &ans[M*M]);
        printf("Writing beta\n");
        write_matrix_j("BetasMPI.mat", M, &ans[M*M]);
        printf("Writing gamma\n");
        write_matrix("GammasMPI.mat", M, ans);
    }
    printf("Thread %d finish.\n", world_rank);
    
    MPI_Finalize();
    return 0;
}


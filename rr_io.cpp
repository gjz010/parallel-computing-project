#include <iostream>
#include <cstdlib>
extern "C"{
    void read_matrix(const char* path, int M, int N, double* mat){
        ifstream matrixfile(path);
        if(!(matrixfile.is_open())){
            cout<<"Error: file not found"<<endl;
            abort();
        }
        for(int i = 0; i < M; i++){
            for(int j =0; j < N; j++){
                matrixfile >> mat[i*N+j];
            }
        }

        matrixfile.close();
    }

    void write_matrix(const char* path, int M, int N, const double* mat){

    }

    void write_matrix_i(const char* path, int M, int N, const double* mat){

    }
    void write_matrix_j(const char* path, int M, int N, const double* mat){

    }
}
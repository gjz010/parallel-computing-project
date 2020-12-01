#include <iostream>
#include <fstream>
#include <cstdlib>
using namespace std;
extern "C"{
    void read_matrix(const char* path, int M, int N, int Npadded, double* mat){
        ifstream matrixfile(path);
        if(!(matrixfile.is_open())){
            cout<<"Error: file not found"<<endl;
            abort();
        }
        for(int i = 0; i < M; i++){
            for(int j =0; j < N; j++){
                matrixfile >> mat[i*Npadded+j];
            }
        }

        matrixfile.close();
    }

    void write_matrix(const char* path, int M, const double* mat){
            ofstream f;
            //file for Matrix A
            f.open(path); 
            for(int i = 0; i<M;i++){
                for(int j =0; j<M;j++){
                    f<<" "<<mat[i*M+j];
                }
                f<<"\n";
            }
            f.close();
    }

    void write_matrix_i(const char* path, int M, const double* mat){
            ofstream f;
            f.open(path); 
            for(int i = 0; i<M;i++){
                for(int j =0; j<M;j++){
                    f<<" "<<mat[i];
                }
                f<<"\n";
            }
            f.close();
    }
    void write_matrix_j(const char* path, int M, const double* mat){
            ofstream f;
            f.open(path); 
            for(int i = 0; i<M;i++){
                for(int j =0; j<M;j++){
                    f<<" "<<mat[j];
                }
                f<<"\n";
            }
            f.close();
    }
}
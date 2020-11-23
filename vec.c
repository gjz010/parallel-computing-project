#include "vec.h"
#include <stdlib.h>
void vec_push(vec_int* v, int val){
    if(v->size==v->cap){
        if(v->cap==0){
            v->cap=1;
            v->data=malloc(sizeof(int));
        }else{
            v->cap=v->cap*2;
            v->data=realloc(v->data, v->cap*sizeof(int));
        }
    }
    v->data[v->size]=val;
    v->size++;
}

void vec_destroy(vec_int* v){
    free(v->data);
}
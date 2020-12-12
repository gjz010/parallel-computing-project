#ifndef _VEC_H
#define _VEC_H
#include <stddef.h>
typedef struct vec_int{
    int* data;
    size_t size;
    size_t cap;
} vec_int;

#define NEW_VEC {NULL,0,0}

void vec_push(vec_int* v, int val);
void vec_destroy(vec_int* v);
#endif

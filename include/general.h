#ifndef GENERAL_H
#define GENERAL_H

#include <stdlib.h>
#include <stdio.h>

void* s_malloc(size_t size){
    void* ptr = malloc(size);
    if (ptr == NULL){
        fprintf(stderr, "Fatal: Could not allocate memory (realloc returned NULL)!");
        exit(1);
    }
    return ptr;
}

void* s_realloc(void* ptr, size_t size){
    ptr = realloc(ptr, size);
    if (ptr == NULL){
        fprintf(stderr, "Fatal: Could not allocate memory (realloc returned NULL)!");
        exit(1);
    }
    return ptr;
}

#endif


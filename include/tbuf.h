#ifndef TRACKED_BUF_H
#define TRACKED_BUF_H

#include <string.h>
#include "general.h"

typedef struct src_pos_t{
    int line;
    int column;
} src_pos_t;

typedef struct tbuf_t{
    char* buffer;                 
    src_pos_t* positions;
    size_t length;
    size_t capacity;
} tbuf_t;

typedef struct{
    tbuf_t** lines;
    size_t len;
    size_t cap;
} line_buf_t;

tbuf_t* tracked_buffer_new(void) {
    tbuf_t* tb = s_malloc(sizeof(tbuf_t));
    tb->capacity = 1024;
    tb->length = 0;
    tb->buffer = s_malloc(tb->capacity);
    tb->positions = s_malloc(tb->capacity * sizeof(src_pos_t));
    return tb;
}

void tracked_buffer_append(tbuf_t* tb, char c, int line, int column) {
    if (tb->length >= tb->capacity) {
        tb->capacity *= 2;
        tb->buffer = s_realloc(tb->buffer, tb->capacity);
        tb->positions = s_realloc(tb->positions, tb->capacity * sizeof(src_pos_t));
    }
    tb->buffer[tb->length] = c;
    tb->positions[tb->length] = (src_pos_t){line, column};
    tb->length++;
}

void tracked_buffer_free(tbuf_t* tb){
    free(tb->buffer);
    free(tb->positions);
    free(tb);
}

#endif
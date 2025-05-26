#ifndef CCC_ERROR_H
#define CCC_ERROR_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <math.h>

#include "general.h"

typedef enum diag_level_t{
    PP_FATAL,
    PP_ERROR, 
    PP_WARNING,
    PP_NOTE
} diag_level_t;

typedef struct diag_t{
    diag_level_t level;
    char* filename;
    int line;
    int col;
    size_t len;
    char* message;
    bool is_owned;
    char** include_stack;
    size_t include_stack_len;
} diag_t;

typedef struct diag_state_t{
    diag_t* errors;
    size_t errorlen;
    size_t errorcap;
} diag_state_t;

typedef struct diag_metadata_t{
    char* filename;
    int line;
    int col;
    char** include_stack;
    size_t include_stack_len;
} diag_metadata_t;

diag_state_t* diag_init(void){
    diag_state_t* diag_state = s_malloc(sizeof(diag_state_t));

    diag_state->errors = s_malloc(sizeof(diag_t) * 8);
    diag_state->errorlen = 0;
    diag_state->errorcap = 8;

    return diag_state;
}

void diag_free(diag_state_t* diag_state){
    for (int i = 0; i < diag_state->errorlen; i++){
        diag_t* diag = &diag_state->errors[i];
        if (diag->is_owned && diag->message != NULL){
            free(diag->message);
        }
    }
    free(diag_state->errors);
    free(diag_state);
    return;
}

void diag_add(diag_state_t* diag_state, diag_metadata_t* metadata, diag_level_t level, char* message, bool is_owned, size_t len){
    if (diag_state->errorlen >= diag_state->errorcap-1){
        diag_state->errors = s_realloc(diag_state->errors, sizeof(diag_t) * diag_state->errorcap*2);
        diag_state->errorcap *= 2;
    }

    diag_state->errors[diag_state->errorlen].level = level;
    diag_state->errors[diag_state->errorlen].filename = metadata->filename;
    diag_state->errors[diag_state->errorlen].line = metadata->line;
    diag_state->errors[diag_state->errorlen].col = metadata->col;
    diag_state->errors[diag_state->errorlen].message = message;
    diag_state->errors[diag_state->errorlen].is_owned = is_owned;
    diag_state->errors[diag_state->errorlen].len = len;
    diag_state->errors[diag_state->errorlen].include_stack = metadata->include_stack;
    diag_state->errors[diag_state->errorlen].include_stack_len = metadata->include_stack_len;

    diag_state->errorlen++;
    return;
}

void diag_display_content(diag_t* diag) {
    if (diag->level == PP_FATAL) {
        return;
    }

    FILE* err_fd = fopen(diag->filename, "r");
    if (!err_fd) {
        return; // or optionally report internal error
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    int cur_line = 1;

    while ((read = getline(&line, &len, err_fd)) != -1) {
        if (cur_line == diag->line) {
            break;
        }
        cur_line++;
    }

    // Strip trailing newline (optional)
    if (read > 0 && line[read - 1] == '\n') {
        line[read - 1] = '\0';
    }

    // Print the line
    fprintf(stderr, "    %d |    %s\n", diag->line, line);

    // Print spaces and caret
    int lineno_width = (diag->line == 0) ? 1 : (int)log10(diag->line) + 1;
    for (int i = 0; i < 5 + lineno_width; i++) putc(' ', stderr); // indent before `|`
    putc('|', stderr);

    for (int i = 0; i < diag->col + 3; i++) putc(' ', stderr); // position the caret
    putc('^', stderr);
    for (int i = 0; i < diag->len-1; i++) putc('~', stderr);
    fputs("\n\n", stderr);

    free(line);
}

char* make_include_stack_str(char** include_stack, size_t include_stack_len) {
    if (include_stack_len == 0) return "";
    const char annotation[] = "In file included from %s:\n";
    char* result = s_malloc(sizeof(char) * 256);
    size_t result_cap = 256;
    int pos = 0;
    for (int i = 0; i < include_stack_len; i++) {
        size_t len = snprintf(NULL, 0, annotation, include_stack[i]);
        pos += len;
        if (pos >= result_cap - 1) {
            result_cap *= 2;
            result = s_realloc(result, result_cap);
        }
        snprintf(result+pos-len, len+1, annotation, include_stack[i]);
    }
    return result;
}

void diag_display(diag_state_t* diag_state){
    const char* color_error = "\033[1;31m";
    const char* color_warning = "\033[1;33m";
    const char* color_note = "\033[1;34m";
    const char* color_reset = "\033[0m";

    for (size_t i = 0; i < diag_state->errorlen; ++i) {
        diag_t* diag = &diag_state->errors[i];
        const char* level_str = NULL;
        const char* color = NULL;

        switch (diag->level) {
            case PP_FATAL:
                level_str = "Fatal Error";
                color = color_error;
            case PP_ERROR:
                level_str = "Error";
                color = color_error;
                break;
            case PP_WARNING:
                level_str = "Warning";
                color = color_warning;
                break;
            case PP_NOTE:
                level_str = "Note";
                color = color_note;
                break;
            default:
                level_str = "Unknown";
                color = color_reset;
        }
        char* include_stack_str = make_include_stack_str(diag->include_stack, diag->include_stack_len);

        fprintf(stderr, "%s%s%s:%d:%d: %s: %s%s\n",
            color,
            include_stack_str,
            diag->filename ? diag->filename : "<unknown>",
            diag->line,
            diag->col,
            level_str,
            diag->message ? diag->message : "",
            color_reset
        );

        if (diag->include_stack_len > 0) {
            free(include_stack_str);
        }

        diag_display_content(diag);
    }
}

#endif


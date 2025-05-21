// Written by AI

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../../src/tokenize.c"
#include "../../../include/error.h"

// Helper to compare two strings and print a diff if they differ
bool compare_and_report(const char* actual, const char* expected, const char* label) {
    if (strcmp(actual, expected) == 0) {
        printf("[PASS] %s\n", label);
        return true;
    } else {
        printf("[FAIL] %s\nExpected:\n%s\nActual:\n%s\n", label, expected, actual);
        return false;
    }
}

const char test_content[] =
    "this is a test\n"
    "This line should be merged \\\n"
    "with this line\n";

const char test_expected[] =
    "this is a test\n"
    "This line should be merged with this line\n";

// Test: make_lines_c (line splicing from char* input)
void test_make_lines_c() {
    line_buf_t* lines = make_lines_c((char*)test_content);

    // Flatten lines into a single string
    size_t total_len = 0;
    for (size_t i = 0; i < lines->len; ++i) {
        total_len += lines->lines[i]->length;
    }
    char* result = malloc(total_len + 1);
    size_t pos = 0;
    for (size_t i = 0; i < lines->len; ++i) {
        memcpy(result + pos, lines->lines[i]->buffer, lines->lines[i]->length);
        pos += lines->lines[i]->length;
    }
    result[total_len] = '\0';

    compare_and_report(result, test_expected, "make_lines_c (char* input)");

    free(result);
    for (size_t i = 0; i < lines->len; ++i) {
        tracked_buffer_free(lines->lines[i]);
    }
    free(lines->lines);
    free(lines);
}

// Test: make_lines_tb (line splicing from tbuf_t input)
void test_make_lines_tb() {
    // Fill tbuf_t with test_content
    tbuf_t* tb = tracked_buffer_new();
    int line = 1, col = 1;
    for (size_t i = 0; test_content[i] != '\0'; ++i) {
        tracked_buffer_append(tb, test_content[i], line, col);
        if (test_content[i] == '\n') {
            line++;
            col = 1;
        } else {
            col++;
        }
    }

    line_buf_t* lines = make_lines_tb(tb);

    // Flatten lines into a single string
    size_t total_len = 0;
    for (size_t i = 0; i < lines->len; ++i) {
        total_len += lines->lines[i]->length;
    }
    char* result = malloc(total_len + 1);
    size_t pos = 0;
    for (size_t i = 0; i < lines->len; ++i) {
        memcpy(result + pos, lines->lines[i]->buffer, lines->lines[i]->length);
        pos += lines->lines[i]->length;
    }
    result[total_len] = '\0';

    compare_and_report(result, test_expected, "make_lines_tb (tbuf_t input)");

    free(result);
    for (size_t i = 0; i < lines->len; ++i) {
        tracked_buffer_free(lines->lines[i]);
    }
    free(lines->lines);
    free(lines);
    tracked_buffer_free(tb);
}

int main(void){
    test_make_lines_c();
    test_make_lines_tb();
    return 0;
}
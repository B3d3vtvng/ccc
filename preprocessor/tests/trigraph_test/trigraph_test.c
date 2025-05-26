// Written by AI because I am lazy :)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../../src/tokenize.c"
#include "../../../include/error.h"

// Helper to compare two strings and print a diff if they differ
bool compare_and_report(const tbuf_t* actual, const char* expected, const char* label) {
    if (strncmp(actual->buffer, expected, actual->length-1) == 0) {
        printf("[PASS] %s\n", label);
        return true;
    } else {
        actual->buffer[actual->length] = '\0';
        printf("[FAIL] %s\nExpected:\n%s\nActual:\n%s\n", label, expected, actual->buffer);
        return false;
    }
}

// Test 1: All trigraphs should be replaced
void test_trigraphs_valid(void) {

    // Expected output: manually substitute trigraphs in the test1.txt file
    const char* expected = "#[]{}\\^|~";
    char* input = "??=??(??)??<??>??/??'??!??-";

    diag_state_t* diag_state = diag_init();
    diag_metadata_t metadata = { .filename = "tests/trigraph_test/trigraph_test1", .line = 1, .col = 1, .include_stack = NULL, .include_stack_len = 0 };

    tbuf_t* output_buf = handle_trigraph_substitution(input, &metadata, diag_state);

    compare_and_report(output_buf, expected, "Trigraphs valid substitution");

    if (diag_state->errorlen != 0) {
        printf("[FAIL] Expected no errors, got %zu\n", diag_state->errorlen);
        diag_display(diag_state);
    } else {
        printf("[PASS] No errors for valid trigraphs\n");
    }

    tracked_buffer_free(output_buf);
    diag_free(diag_state);
}

// Test 2: Invalid trigraphs should produce errors
void test_trigraphs_invalid(void) {

    // Expected output: ??<??>??h ?? → { } h ??
    const char* expected = "{ }h ";
    char* input = "??< ??>??h ??";

    diag_state_t* diag_state = diag_init();
    diag_metadata_t metadata = { .filename = "tests/trigraph_test/trigraph_test2", .line = 1, .col = 1, .include_stack = NULL, .include_stack_len = 0 };

    tbuf_t* output_buf = handle_trigraph_substitution(input, &metadata, diag_state);

    compare_and_report(output_buf, expected, "Trigraphs invalid substitution");

    if (diag_state->errorlen == 2) {
        printf("[PASS] Two errors reported for invalid trigraph\n");
        diag_display(diag_state);
    } else {
        printf("[FAIL] Expected 2 errors, got %zu\n", diag_state->errorlen);
        diag_display(diag_state);
    }

    tracked_buffer_free(output_buf);
    diag_free(diag_state);
}

int main(void) {
    test_trigraphs_valid();
    test_trigraphs_invalid();
    return 0;
}

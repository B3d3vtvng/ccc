#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../../src/tokenize.c"
#include "../../../include/error.h"

#define BLUE   "\033[34m"
#define RESET  "\033[0m"

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

// Helper to print token types as strings
const char* token_type_str(pp_tokentype_t t) {
    switch (t) {
        case TOK_IDENTIFIER: return "IDENT";
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING_LIT: return "STRING";
        case TOK_CHAR_LIT: return "CHAR";
        case TOK_OPERATOR: return "OP";
        case TOK_DIRECTIVE: return "DIRECTIVE";
        case TOK_HASH: return "HASH";
        case TOK_DOUBLE_HASH: return "DOUBLE_HASH";
        case TOK_EOL: return "EOL";
        case TOK_WHITESPACE: return "WS";
        case TOK_EOF: return "EOF";
        default: return "???";
    }
}

// Helper to flatten token list to a string for easy comparison
void flatten_tokens(pp_token_list_t* toks, char* out, size_t outcap) {
    size_t pos = 0;
    for (size_t i = 0; i < toks->token_len && pos < outcap-1; ++i) {
        const char* tstr = token_type_str(toks->tokens[i]->tokentype);
        int n = snprintf(out+pos, outcap-pos, "<%s:%s>", tstr, toks->tokens[i]->value ? toks->tokens[i]->value->buffer : "NULL");
        pos += n;
    }
    out[pos] = 0;
}

// Helper to check error count and error messages
bool check_errors(diag_state_t* diag, size_t expected_count, const char* label) {
    if (diag->errorlen == expected_count) {
        printf("[PASS] %s error count (%zu)\n", label, expected_count);
        diag_display(diag);
        return true;
    } else {
        printf("[FAIL] %s error count: expected %zu, got %zu\n", label, expected_count, diag->errorlen);
        diag_display(diag);
        return false;
    }
}

// Test: normal tokenization
void test_tokenize_normal(void) {
    const char* input =
        "#include \"test.h\"\n"
        "int main() {\n"
        "  int x = 42;\n"
        "  x = x + 1;\n"
        "  // comment\n"
        "  return x;\n"
        "}\n";

    diag_state_t* diag = diag_init();

    pp_token_list_t* toks = tokenize("tests/tokenize_test/test_normal.c", strdup(input), false, NULL, 0, diag);

    printf(BLUE "[DEBUG] Tokens: " RESET);
    pp_tok_list_display(toks);

    char flat[1024];
    flatten_tokens(toks, flat, sizeof(flat));
    // Just check for some key tokens
    bool ok = strstr(flat, "<IDENT:int>") && strstr(flat, "<IDENT:main>") && strstr(flat, "<NUMBER:42>");
    printf("%s test_tokenize_normal\n", ok ? "[PASS]" : "[FAIL]");
    check_errors(diag, 0, "tokenize_normal");

    diag_free(diag);
    free(toks->tokens);
    free(toks);
}

// Test: unterminated string literal
void test_tokenize_unterminated_string(void) {
    const char* input = "char* s = \"unterminated;\n";
    diag_state_t* diag = diag_init();

    char** include_stack = malloc(sizeof(char*) * 1);
    include_stack[0] = strdup("tests/tokenize_test/super_test.c");

    pp_token_list_t* toks = tokenize("tests/tokenize_test/test_unterm_string.c", strdup(input), false, include_stack, 1, diag);

    printf(BLUE "[DEBUG] Tokens: " RESET);
    pp_tok_list_display(toks);

    check_errors(diag, 1, "unterminated string");
    if (diag->errorlen > 0 && strstr(diag->errors[0].message, "Unterminated string literal"))
        printf("[PASS] unterminated string error message\n");
    else
        printf("[FAIL] unterminated string error message\n");

    diag_free(diag);
    free(toks->tokens);
    free(toks);
}

// Test: unterminated char literal
void test_tokenize_unterminated_char(void) {
    const char* input = "char c = 'a;\n";
    diag_state_t* diag = diag_init();
    pp_token_list_t* toks = tokenize("tests/tokenize_test/test_unterm_char.c", strdup(input), false, NULL, 0, diag);

    printf(BLUE "[DEBUG] Tokens: " RESET);
    pp_tok_list_display(toks);

    check_errors(diag, 1, "unterminated char");
    if (diag->errorlen > 0 && strstr(diag->errors[0].message, "Unterminated character literal"))
        printf("[PASS] unterminated char error message\n");
    else
        printf("[FAIL] unterminated char error message\n");

    diag_free(diag);
    free(toks->tokens);
    free(toks);
}

// Test: invalid token
void test_tokenize_invalid_token(void) {
    const char* input = "int $foo = 1;\n";
    diag_state_t* diag = diag_init();
    pp_token_list_t* toks = tokenize("tests/tokenize_test/test_invalid_token.c", strdup(input), false, NULL, 0, diag);

    printf(BLUE "[DEBUG] Tokens: " RESET);
    pp_tok_list_display(toks);

    check_errors(diag, 1, "invalid token");
    if (diag->errorlen > 0 && strstr(diag->errors[0].message, "Invalid token"))
        printf("[PASS] invalid token error message\n");
    else
        printf("[FAIL] invalid token error message\n");

    diag_free(diag);
    free(toks->tokens);
    free(toks);
}

// Test: invalid number (multiple dots)
void test_tokenize_invalid_number(void) {
    const char* input = "double x = 1.2.3;\n";
    diag_state_t* diag = diag_init();
    pp_token_list_t* toks = tokenize("tests/tokenize_test/test_invalid_number.c", strdup(input), false, NULL, 0, diag);

    printf(BLUE "[DEBUG] Tokens: " RESET);
    pp_tok_list_display(toks);

    check_errors(diag, 1, "invalid number");
    if (diag->errorlen > 0 && strstr(diag->errors[0].message, "Invalid numerical literal"))
        printf("[PASS] invalid number error message\n");
    else
        printf("[FAIL] invalid number error message\n");

    diag_free(diag);
    free(toks->tokens);
    free(toks);
}

// Test: unterminated multi-line comment
void test_tokenize_unterminated_comment(void) {
    const char* input = "int main() { /* comment...\n";
    diag_state_t* diag = diag_init();
    pp_token_list_t* toks = tokenize("tests/tokenize_test/test_unterm_comment.c", strdup(input), false, NULL, 0, diag);

    printf(BLUE "[DEBUG] Tokens: " RESET);
    pp_tok_list_display(toks);

    check_errors(diag, 1, "unterminated comment");
    if (diag->errorlen > 0 && strstr(diag->errors[0].message, "Unterminated multi-line comment"))
        printf("[PASS] unterminated comment error message\n");
    else
        printf("[FAIL] unterminated comment error message\n");

    diag_free(diag);
    free(toks->tokens);
    free(toks);
}

// Test: directive tokenization
void test_tokenize_directive(void) {
    const char* input = "#define FOO 1\n";
    diag_state_t* diag = diag_init();
    pp_token_list_t* toks = tokenize("tests/tokenize_test/test_directive.c", strdup(input), false, NULL, 0, diag);

    printf(BLUE "[DEBUG] Tokens: " RESET);
    pp_tok_list_display(toks);

    char flat[256];
    flatten_tokens(toks, flat, sizeof(flat));
    bool ok = strstr(flat, "<DIRECTIVE:#define>");
    printf("%s test_tokenize_directive\n", ok ? "[PASS]" : "[FAIL]");
    check_errors(diag, 0, "directive");

    diag_free(diag);
    free(toks->tokens);
    free(toks);
}

// Test: double hash tokenization
void test_tokenize_double_hash(void) {
    const char* input = "a ## b\n";
    diag_state_t* diag = diag_init();
    pp_token_list_t* toks = tokenize("tests/tokenize_test/test_double_hash.c", strdup(input), false, NULL, 0, diag);

    printf(BLUE "[DEBUG] Tokens: " RESET);
    pp_tok_list_display(toks);

    char flat[256];
    flatten_tokens(toks, flat, sizeof(flat));
    bool ok = strstr(flat, "<DOUBLE_HASH:##>");
    printf("%s test_tokenize_double_hash\n", ok ? "[PASS]" : "[FAIL]");
    check_errors(diag, 0, "double_hash");

    diag_free(diag);
    free(toks->tokens);
    free(toks);
}

// Test: whitespace tokenization
void test_tokenize_whitespace(void) {
    const char* input = "a \t b\n";
    diag_state_t* diag = diag_init();
    pp_token_list_t* toks = tokenize("tests/tokenize_test/test_ws.c", strdup(input), false, NULL, 0, diag);

    printf(BLUE "[DEBUG] Tokens: " RESET);
    pp_tok_list_display(toks);

    char flat[256];
    flatten_tokens(toks, flat, sizeof(flat));
    bool ok = strstr(flat, "<WS: \t >");
    printf("%s test_tokenize_whitespace\n", ok ? "[PASS]" : "[FAIL]");
    check_errors(diag, 0, "whitespace");

    diag_free(diag);
    free(toks->tokens);
    free(toks);
}

int main(void) {
    test_tokenize_normal();
    test_tokenize_unterminated_string();
    test_tokenize_unterminated_char();
    test_tokenize_invalid_token();
    test_tokenize_invalid_number();
    test_tokenize_unterminated_comment();
    test_tokenize_directive();
    test_tokenize_double_hash();
    test_tokenize_whitespace();
    return 0;
}
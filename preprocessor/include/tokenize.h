#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "token.h"
#include "../../include/error.h"
#include "../../include/general.h"
#include "../../include/tbuf.h"

typedef enum {
    MACRO_DEFINE,
    MACRO_UNDEF
} macro_action_t;

typedef struct {
    macro_action_t action;
    const char* name;
    const char* value; // NULL for MACRO_UNDEF
} macro_entry_t;

typedef struct {
    char* filename;             // For diagnostics
    char* input;                // Optional: file contents; if NULL, read from filename

    bool enable_trigraphs;            // Enable trigraph translation

    char** include_paths;      // -I paths
    size_t include_path_count;

    char** pre_includes;       // -include files (actual includes)
    size_t pre_include_count;

    char** pre_macro_includes; // -imacros files (only macros included)
    size_t pre_macro_include_count;

    char** include_stack;
    size_t include_stack_len;
} pp_tokenizer_context_t;


token_list_t* tokenize(pp_tokenizer_context_t* context, diag_state_t* diag_state);

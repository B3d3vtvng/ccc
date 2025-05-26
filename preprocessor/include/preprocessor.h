#include "../../include/tbuf.h"
#include "../../include/error.h"

typedef struct pp_macro_entry_t {
    char* name;
    tbuf_t* body;
    src_pos_t** subst_positions;
} pp_macro_entry_t;

typedef struct pp_include_entry_t {
    char** include_stack;
    size_t include_stack_len;
    char* filename;
    src_pos_t pos_start;
    src_pos_t pos_end;
} pp_include_entry_t;

typedef struct pp_macro_metadata_t {
    pp_macro_entry_t* macro_metadata;
    size_t len;
} pp_macro_metadata_t;

typedef struct pp_include_metadata_t {
    pp_include_entry_t* include_metadata;
    size_t len;
} pp_include_metadata_t;

typedef struct pp_metadata_t {
    pp_macro_metadata_t* macro_metadata;
    pp_include_metadata_t* include_metadata;
} diag_metadata_t;

typedef struct pp_res_t {
    tbuf_t* output;
    pp_metadata_t* metadata;
} pp_res_t;

typedef enum pp_macro_action_t {
    MACRO_DEFINE,
    MACRO_UNDEF
} pp_macro_action_t;

typedef struct pp_macro_entry_t {
    pp_macro_action_t action;
    const char* name;
    const char* value; // NULL for MACRO_UNDEF
} pp_macro_entry_t;

typedef struct pp_context_t {
    char* filename;             // For diagnostics
    char* input;                // Optional: file contents; if NULL, read from filename

    bool enable_trigraphs;            // Enable trigraph translation

    char** include_paths;      // -I paths
    size_t include_path_count;

    char** pre_includes;       // -include files (actual includes)
    size_t pre_include_count;

    char** pre_macro_includes; // -imacros files (only macros included)
    size_t pre_macro_include_count;
} pp_context_t;

pp_res_t* preprocesser_run(pp_context_t* context, diag_state_t* diag_state);
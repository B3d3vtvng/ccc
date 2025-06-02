#ifndef METADATA_H
#define METADATA_H

#include "../../include/tbuf.h"
#include "../include/token.h"
#include <stdbool.h>

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
    pp_macro_metadata_t* active_macros;
    pp_include_metadata_t* include_metadata;
} pp_metadata_t;

typedef enum pp_include_type_t {
    PP_INCL_SYSTEM,
    PP_INCL_EXPLICIT,
    PP_INCL_LOCAL
} pp_include_type_t;

typedef enum pp_incl_directive_t{
    PP_INCL_DIRECTIVE_LOCAL,
    PP_INCL_DIRECTIVE_SYS
} pp_incl_directive_t;

typedef struct pp_include_path_t {
    char* path;
    pp_include_type_t type;
} pp_include_path_t;

typedef struct pp_context_t {
    char* filename;             // For diagnostics
    char* input;   
    
    char** include_stack;
    size_t include_stack_len;  // For diagnostics
    size_t include_stack_cap;   // For diagnostics

    pp_include_path_t** include_paths;       // Paths to search for included files
    size_t include_path_count;  // Number of include paths
    size_t include_path_cap;    // Capacity of include paths array

    bool enable_trigraphs;            // Enable trigraph translation
} pp_context_t;

typedef struct pp_res_t {
    tbuf_t* output;
    pp_metadata_t* metadata;
} pp_res_t;

typedef struct pp_eval_ret_t {
    pp_token_list_t* tokens;
    pp_metadata_t* metadata;
} pp_eval_ret_t;

#endif

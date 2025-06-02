#include "../include/preprocessor.h"

tbuf_t* pp_construct_output(pp_token_list_t* tokens){
    tbuf_t* output = tracked_buffer_new();

    if (tokens->token_len == 0) return output;

    for (int i = 0; i < tokens->token_len; i++){

        if (tokens->tokens[i]->value == NULL && tokens->tokens[i]->tokentype == TOK_EOF) continue;
        else if (tokens->tokens[i]->value == NULL && tokens->tokens[i]->tokentype == TOK_EOL){
            tracked_buffer_append(output, '\n', -1, -1);
            continue;
        }

        for (int j = 0; j < tokens->tokens[i]->value->length; j++){
            tracked_buffer_append(output, tokens->tokens[i]->value->buffer[j], tokens->tokens[i]->value->positions[j].line, tokens->tokens[i]->value->positions[j].column);
        }
    }

    return output;
}

int eval_define(pp_token_list_t* tokens, 
    int* i, 
    diag_metadata_t* metadata, 
    pp_macro_metadata_t* macro_entries, 
    size_t* macro_cap, 
    pp_macro_metadata_t* macro_metadata,
    size_t* macro_meta_cap,
    diag_state_t* diag_state){

    return 0;
}

int eval_undef(pp_token_list_t* tokens,
    int* i, 
    diag_metadata_t* metadata, 
    pp_macro_metadata_t* macro_entries, 
    diag_state_t* diag_state){

    return 0;
}

int eval_conditional(pp_token_list_t* tokens,
    int* i, 
    diag_metadata_t* metadata, 
    pp_macro_metadata_t* macro_entries, 
    pp_token_list_t* output_tokens, 
    diag_state_t* diag_state){

    return 0;
}

int eval_error(pp_token_list_t* tokens, 
    int* i, 
    diag_metadata_t* metadata, 
    diag_state_t* diag_state){

    return 0;
}

int eval_pragma(pp_token_list_t* tokens, 
    int* i, 
    diag_metadata_t* metadata, 
    diag_state_t* diag_state){

    return 0;
}

int eval_line(pp_token_list_t* tokens, 
    int* i, 
    diag_metadata_t* metadata, 
    pp_context_t* context,
    diag_state_t* diag_state){

    return 0;
}

char** include_stack_deepcopy(char** include_stack, size_t include_stack_len) {
    char** new_stack = s_malloc(sizeof(char*) * include_stack_len);
    for (size_t i = 0; i < include_stack_len; i++) {
        new_stack[i] = strdup(include_stack[i]);
    }
    return new_stack;
}

int handle_include_path_extension(char* full_path, char* filename, pp_include_path_t** include_paths, size_t* include_path_count, size_t* include_path_cap, bool is_system_include) {
    int last_path_sep = -1;
    int i = 0;
    while (i < strlen(full_path)) {
        if (full_path[i] == '/' || full_path[i] == '\\') {
            last_path_sep = i;
        }
        i++;
    }
    if (last_path_sep == -1) {
        // No path separator found, return
        return -1;
    }

    pp_include_path_t* include_path = s_malloc(sizeof(pp_include_path_t));
    include_path->path = s_malloc(last_path_sep + 1);
    if (is_system_include) {
        include_path->type = PP_INCL_SYSTEM;
    } else {
        include_path->type = PP_INCL_LOCAL;
    }

    strncpy(include_path->path, full_path, last_path_sep);

    // Check if the path is already in the include paths
    for (size_t i = 0; i < *include_path_count; i++) {
        if (strcmp(include_paths[i]->path, include_path->path) == 0) {
            free(include_path->path);
            free(include_path);
            return -1; // Path already exists
        }
    }

    // Add the new include path
    if (*include_path_count >= *include_path_cap) {
        *include_path_cap *= 2;
        include_paths = s_realloc(include_paths, sizeof(pp_include_path_t*) * (*include_path_cap));
    }

    include_paths[*include_path_count] = include_path;
    (*include_path_count)++;
    return 0; // Success
}

int handle_include(pp_incl_directive_t include_type,
    char* filename,
    pp_context_t* context,
    diag_metadata_t* metadata,
    size_t line_len,
    pp_macro_metadata_t* macro_entries,
    size_t* macro_cap,
    pp_token_list_t* output_tokens,
    pp_macro_metadata_t* macro_metadata,
    size_t* macro_meta_cap,
    pp_include_metadata_t* include_metadata,
    size_t* include_cap,
    diag_state_t* diag_state) {

    // Check if the file is already included
    for (size_t j = 0; j < context->include_stack_len; j++) {
        if (strcmp(context->include_stack[j], filename) == 0) {
            diag_add(diag_state, metadata, PP_ERROR, "Circular include detected", false, line_len);
            return -1; // Circular include detected
        }
    }

    // Add the file to the include metadata
    if (include_metadata->len >= *include_cap) {
        *include_cap *= 2;
        include_metadata->include_metadata = realloc(include_metadata->include_metadata, sizeof(pp_include_entry_t) * (*include_cap));
    }

    pp_include_entry_t new_entry;
    new_entry.filename = strdup(filename);
    new_entry.include_stack = include_stack_deepcopy(context->include_stack, context->include_stack_len);
    new_entry.include_stack_len = context->include_stack_len;
    new_entry.pos_start = (src_pos_t){metadata->line, metadata->col}; // Placeholder, will be set later

    char* full_path = NULL;
    
    for (int i = 0; i < context->include_path_count; i++){
        if (include_type == PP_INCL_DIRECTIVE_SYS && context->include_paths[i]->type != PP_INCL_SYSTEM) continue;
        full_path = s_malloc(strlen(context->include_paths[i]->path) + strlen(filename) + 2);
        sprintf(full_path, "%s/%s", context->include_paths[i]->path, filename);

        FILE* file = fopen(full_path, "r");
        if (file != NULL) {
            fclose(file);
            break; // File found
        }
        free(full_path);
        full_path = NULL;
    }

    if (full_path == NULL) {
        long err_len = snprintf(NULL, 0, "Could not find included file '%s'", filename);
        char* err_msg = s_malloc(err_len + 1);
        snprintf(err_msg, err_len + 1, "Could not find included file '%s'", filename);
        diag_add(diag_state, metadata, PP_ERROR, err_msg, true, line_len);
        return -2;
    }

    bool has_new_include_path = false;
    if (handle_include_path_extension(full_path, filename, context->include_paths, &context->include_path_count, &context->include_path_cap, (include_type == PP_INCL_DIRECTIVE_SYS)) == 0){
        has_new_include_path = true;
    }

    // Update the context's include stack
    context->include_stack[context->include_stack_len++] = context->filename;
    pp_token_list_t* included_tokens = tokenize(full_path, NULL, context->enable_trigraphs, context->include_stack, context->include_stack_len, diag_state);

    if (included_tokens == NULL) {
        return -2; // Fatal Error during tokenization
    }

    if (included_tokens->token_len < 3){
        context->include_stack_len--;
        pp_tok_list_free(included_tokens);
        return 0;
    }

    tbuf_t* last_tok_value = included_tokens->tokens[included_tokens->token_len - 3]->value;
    new_entry.pos_end = last_tok_value->positions[last_tok_value->length-1]; // Set end position to last chars position
    include_metadata->include_metadata[include_metadata->len++] = new_entry;

    // Set new filename but save old one
    char* cur_filename = context->filename;
    context->filename = full_path;

    pp_eval_ret_t* eval_result = pp_evaluate_tokens(
        included_tokens,
        context,
        diag_state
    );
    
    context->filename = cur_filename; // Restore old filename

    if (eval_result == NULL) {
        pp_tok_list_free(included_tokens);
        return -2; // Fatal Error during evaluation
    }

    // Append the evaluated tokens to the output tokens
    for (int i = 0; i < eval_result->tokens->token_len; i++){
        output_tokens->tokens[output_tokens->token_len++] = eval_result->tokens->tokens[i];
        if (output_tokens->token_len == output_tokens->token_cap){
            output_tokens->token_cap *= 2;
            output_tokens->tokens = s_realloc(output_tokens->tokens, sizeof(pp_token_t*) * output_tokens->token_cap);
        }
    }

    //Append the generated active macros to the macro list
    for (int i = 0; i < eval_result->metadata->active_macros->len; i++){
        macro_entries->macro_metadata[macro_entries->len++] = eval_result->metadata->active_macros->macro_metadata[i];
        if (macro_entries->len == *macro_cap){
            *macro_cap *= 2;
            macro_entries->macro_metadata = s_realloc(macro_entries->macro_metadata, *macro_cap);
        }
    }

    //Append the generated macro metadata to the macro metadata
    for (int i = 0; i < eval_result->metadata->macro_metadata->len; i++){
        macro_metadata->macro_metadata[macro_entries->len++] = eval_result->metadata->macro_metadata->macro_metadata[i];
        if (macro_metadata->len == *macro_meta_cap){
            *macro_meta_cap *= 2;
            macro_metadata->macro_metadata = s_realloc(macro_metadata->macro_metadata, *macro_meta_cap);
        }
    }

    // Append the generated include metadata to the include metadata
    for (int i = 0; i < eval_result->metadata->include_metadata->len; i++){
        include_metadata->include_metadata[include_metadata->len++] = eval_result->metadata->include_metadata->include_metadata[i];
        if (include_metadata->len == *include_cap){
            *include_cap *= 2;
            include_metadata->include_metadata = s_realloc(include_metadata->include_metadata, *include_cap);
        }
    }

    if (has_new_include_path){
        // If we added a new include path, we need to free it
        free(context->include_paths[context->include_path_count-1]->path);
        free(context->include_paths[context->include_path_count-1]);
        context->include_path_count--;
    }

    context->include_stack_len--;
    free(eval_result->tokens->tokens);
    free(eval_result->tokens);
    free(eval_result->metadata->active_macros);
    free(eval_result->metadata->macro_metadata);
    free(eval_result->metadata->include_metadata);
    free(eval_result);
    return 0;
}

int eval_include(
    pp_token_list_t* tokens,
    int* i,
    diag_metadata_t* metadata,
    pp_context_t* context,
    pp_macro_metadata_t* macro_entries,
    size_t* macro_cap,
    pp_token_list_t* output_tokens,
    pp_macro_metadata_t* macro_metadata,
    size_t* macro_meta_cap,
    pp_include_metadata_t* include_metadata,
    size_t* include_cap,
    diag_state_t* diag_state){

    // Capture whole line of tokens after the #include directive
    pp_token_list_t* line_tokens = s_malloc(sizeof(pp_token_list_t));
    line_tokens->tokens = s_malloc(sizeof(pp_token_t*) * 64);
    line_tokens->token_len = 0;
    line_tokens->token_cap = 64;

    //Skip whitespace
    (*i)++;

    metadata->col = tokens->tokens[*i]->src_pos.column + tokens->tokens[*i]->value->length + 1;
    metadata->line = tokens->tokens[*i]->src_pos.line;

    // Populate line_tokens with tokens until we hit an EOL
    while (*i < tokens->token_len && tokens->tokens[*i]->tokentype != TOK_EOL) {
        // Skip whitespace tokens as theyre just annoying
        if (tokens->tokens[*i]->tokentype == TOK_WHITESPACE) {
            (*i)++;
            continue;
        }
        if (line_tokens->token_len >= line_tokens->token_cap) {
            line_tokens->token_cap *= 2;
            line_tokens->tokens = realloc(line_tokens->tokens, sizeof(pp_token_t*) * line_tokens->token_cap);
        }
        line_tokens->tokens[line_tokens->token_len++] = tokens->tokens[*i];
        (*i)++;
    }

    // If no tokens were captured, report an error
    if (line_tokens->token_len == 0) {
        metadata->col = tokens->tokens[*i]->src_pos.column;
        metadata->line = tokens->tokens[*i]->src_pos.line;
        diag_add(diag_state, metadata, PP_ERROR, "Expected operant for #include directive", false, 1);
        free(line_tokens->tokens);
        free(line_tokens);
        return -1;
    }

    // Get line len for errorhandling
    size_t line_len = tokens->tokens[*i]->src_pos.column - metadata->col + 1 ;

    // Skip EOL
    if (*i < tokens->token_len && tokens->tokens[*i]->tokentype == TOK_EOL) {
        (*i)++;
    }

    // Handle string includes eg #include "file.h"
    if (line_tokens->token_len == 1){

        if (line_tokens->tokens[0]->tokentype != TOK_STRING_LIT){
            diag_add(diag_state, metadata, PP_ERROR, "Invalid #include directive", false, line_len);
            free(line_tokens->tokens);
            free(line_tokens);
            return -1;
        }

        char* filename = line_tokens->tokens[0]->value->buffer;

        // Free temporary line tokens, but not with pp_tok_list_free, because it also frees the tokens which we dont own
        free(line_tokens->tokens);
        free(line_tokens);

        return handle_include(
            PP_INCL_DIRECTIVE_LOCAL,
            filename,
            context,
            metadata,
            line_len,
            macro_entries,
            macro_cap,
            output_tokens,
            macro_metadata,
            macro_meta_cap,
            include_metadata,
            include_cap,
            diag_state
        );
    }
    // Handle angle bracket includes eg #include <file.h>
    // IMPORTANT: Filename in brackets may not contain a whitespace
    else if (line_tokens->token_len >= 3){
        // Validate correct syntax for angle bracket includes
        if (line_tokens->tokens[0]->tokentype != TOK_OPERATOR || line_tokens->tokens[0]->value->buffer[0] != '<' ||
            line_tokens->tokens[line_tokens->token_len-1]->tokentype != TOK_OPERATOR || line_tokens->tokens[line_tokens->token_len-1]->value->buffer[0] != '>') {
            diag_add(diag_state, metadata, PP_ERROR, "Invalid #include directive", false, line_len);
            free(line_tokens->tokens);
            free(line_tokens);
            return -1;
        }

        pp_token_list_t* header_tokens = s_malloc(sizeof(pp_token_list_t));
        header_tokens->tokens = s_malloc(sizeof(pp_token_t*) * 8);
        header_tokens->token_cap = 8;
        header_tokens->token_len = 0;
        for (int i = 1; !(line_tokens->tokens[i]->tokentype == TOK_OPERATOR && line_tokens->tokens[i]->value->buffer[0] == '>');i++){
            header_tokens->tokens[header_tokens->token_len++] = line_tokens->tokens[i];
            if (header_tokens->token_len >= header_tokens->token_cap){
                header_tokens->token_cap *= 2;
                header_tokens->tokens = s_realloc(header_tokens->tokens, header_tokens->token_cap);
            }
        }   

        char* filename = pp_construct_output(header_tokens)->buffer;

        // Free temporary line tokens, but not with pp_tok_list_free, because it also frees the tokens which we dont own
        free(line_tokens->tokens);
        free(line_tokens);

        // Free temporaty header tokens, same reasoning as above
        free(header_tokens->tokens);
        free(header_tokens);

        return handle_include(
            PP_INCL_DIRECTIVE_SYS,
            filename,
            context,
            metadata,
            line_len,
            macro_entries,
            macro_cap,
            output_tokens,
            macro_metadata,
            macro_meta_cap,
            include_metadata,
            include_cap,
            diag_state
        );
    }
    else {
        diag_add(diag_state, metadata, PP_ERROR, "Invalid #include directive", false, line_len);
        free(line_tokens->tokens);
        free(line_tokens);
        return -1;
    }
}

int eval_directive(
    pp_token_list_t* tokens,
    int* i,
    pp_context_t* context,
    diag_metadata_t* metadata,
    pp_macro_metadata_t* macro_entries,
    size_t* macro_cap,
    pp_token_list_t* output_tokens,
    pp_macro_metadata_t* macro_metadata,
    size_t* macro_meta_cap,
    pp_include_metadata_t* include_metadata,
    size_t* include_cap,
    diag_state_t* diag_state) {

    char* directive = tokens->tokens[*i]->value->buffer;
    if ((int)strcmp(directive, "#include") == 0){
        return eval_include(tokens, 
                i, 
                metadata,
                context,
                macro_entries, 
                macro_cap, 
                output_tokens, 
                macro_metadata,
                macro_meta_cap,
                include_metadata, 
                include_cap,
                diag_state);
    }
    else if (strcmp(directive, "#define") == 0){
        return eval_define(tokens, 
                i, 
                metadata, 
                macro_entries, 
                macro_cap, 
                macro_metadata,
                macro_meta_cap,
                diag_state);
    }
    else if (strcmp(directive, "#undef") == 0){
        return eval_undef(tokens,
                i, 
                metadata, 
                macro_entries, 
                diag_state);
    }
    else if (strcmp(directive, "#if") == 0 
                || strcmp(directive, "#ifdef") == 0 
                || strcmp(directive, "#ifndef") == 0
                || strcmp(directive, "#elif") == 0
                || strcmp(directive, "#else") == 0){
        return eval_conditional(tokens,
                i, 
                metadata, 
                macro_entries, 
                output_tokens, 
                diag_state);
    }
    else if (strcmp(directive, "#error") == 0){
        return eval_error(tokens, 
                i, 
                metadata, 
                diag_state);
    }
    else if (strcmp(directive, "#pragma") == 0){
        return eval_pragma(tokens, 
                i, 
                metadata, 
                diag_state);
    }
    else if (strcmp(directive, "#line") == 0){
        return eval_line(tokens, 
                i, 
                metadata, 
                context, 
                diag_state);
    }
    else if (strcmp(directive, "#endif") == 0){
        (*i)++;
        return 0;
    }
    else {
        metadata->col = tokens->tokens[*i]->src_pos.column;
        metadata->line = tokens->tokens[*i]->src_pos.line;
        diag_add(diag_state, metadata, PP_ERROR, "Unknown preprocessor directive", false, 1);
    }

    return -1;
}

pp_macro_entry_t* find_macro(pp_macro_metadata_t* macro_entries, char* macro_name){
    for (int i = 0; i < macro_entries->len; i++){
        pp_macro_entry_t* entry = &macro_entries->macro_metadata[i];
        if (strcmp(entry->name, macro_name) == 0){
            return entry;
        }
    }

    return NULL;
}

pp_token_list_t* expand_macro(pp_macro_entry_t* macro,
    pp_token_list_t* tokens,
    int* i,
    pp_context_t* context,
    diag_metadata_t* metadata,
    diag_state_t* diag_state){

    return NULL;
}

pp_eval_ret_t* pp_evaluate_tokens(pp_token_list_t* tokens, pp_context_t* context, diag_state_t* diag_state) {
    diag_metadata_t* metadata = s_malloc(sizeof(diag_metadata_t));
    metadata->filename = context->filename;
    metadata->include_stack = context->include_stack;
    metadata->include_stack_len = context->include_stack_len;

    pp_macro_metadata_t* macro_entries = s_malloc(sizeof(pp_macro_entry_t*) * 64);
    macro_entries->macro_metadata = s_malloc(sizeof(pp_macro_entry_t) * 64);
    macro_entries->len = 0;
    size_t macro_cap = 64;

    pp_token_list_t* output_tokens = s_malloc(sizeof(pp_token_list_t));
    output_tokens->tokens = s_malloc(sizeof(pp_token_t*) * 64);
    output_tokens->token_len = 0;
    output_tokens->token_cap = 64;

    pp_token_list_t* discarded_tokens = s_malloc(sizeof(pp_token_list_t));
    discarded_tokens->tokens = s_malloc(sizeof(pp_token_t*) * 64);
    discarded_tokens->token_len = 0;
    discarded_tokens->token_cap = 64;

    pp_include_metadata_t* include_metadata = s_malloc(sizeof(pp_include_metadata_t));
    include_metadata->include_metadata = s_malloc(sizeof(pp_include_entry_t) * 64);
    include_metadata->len = 0;
    size_t include_cap = 64;

    pp_macro_metadata_t* macro_metadata = s_malloc(sizeof(pp_macro_metadata_t));
    macro_metadata->macro_metadata = s_malloc(sizeof(pp_macro_entry_t) * 64);
    macro_metadata->len = 0;
    size_t macro_meta_cap = 64;

    int i = 0;
    while (i < tokens->token_len){
        pp_token_t* token = tokens->tokens[i];
        if (token->tokentype == TOK_DIRECTIVE){
            int eval_directive_result = eval_directive(
                tokens, 
                &i, 
                context, 
                metadata, 
                macro_entries, 
                &macro_cap, 
                output_tokens, 
                macro_metadata,
                &macro_meta_cap,
                include_metadata, 
                &include_cap,
                diag_state
            );
            if (eval_directive_result == -1){
                i++;
                continue;
            }
            if (eval_directive_result == -2){
                free(metadata);
                free(macro_entries->macro_metadata);
                free(macro_entries);
                free(output_tokens);
                free(include_metadata->include_metadata);
                free(include_metadata);
                for (int i = 0; i < macro_metadata->len; i++){
                    tracked_buffer_free(macro_metadata->macro_metadata[i].body);
                    free(macro_metadata->macro_metadata[i].name);
                }
                return NULL; // Fatal error, could not find include file
            }
            while (i < tokens->token_len && tokens->tokens[i]->tokentype != TOK_EOL) {
                discarded_tokens->tokens[discarded_tokens->token_len++] = tokens->tokens[i];
                if (discarded_tokens->token_len >= discarded_tokens->token_cap) {
                    discarded_tokens->token_cap *= 2;
                    discarded_tokens->tokens = realloc(discarded_tokens->tokens, sizeof(pp_token_t*) * discarded_tokens->token_cap);
                }
                i++;
            }
            if (i < tokens->token_len && tokens->tokens[i]->tokentype == TOK_EOL) {
                discarded_tokens->tokens[discarded_tokens->token_len++] = tokens->tokens[i];
                if (discarded_tokens->token_len >= discarded_tokens->token_cap) {
                    discarded_tokens->token_cap *= 2;
                    discarded_tokens->tokens = realloc(discarded_tokens->tokens, sizeof(pp_token_t*) * discarded_tokens->token_cap);
                }
                i++;
            }
            continue;
        }
        else if (token->tokentype == TOK_IDENTIFIER) {
            pp_macro_entry_t* macro = find_macro(macro_entries, token->value->buffer);
            if (macro == NULL){
                if (output_tokens->token_len >= output_tokens->token_cap) {
                    output_tokens->token_cap *= 2;
                    output_tokens->tokens = realloc(output_tokens->tokens, sizeof(pp_token_t*) * output_tokens->token_cap);
                }

                output_tokens->tokens[output_tokens->token_len++] = token;
                i++;
                continue;
            }
            pp_token_list_t* expanded_tokens = expand_macro(
                macro,
                tokens,
                &i,
                context,
                metadata,
                diag_state
            );

            if (expanded_tokens == NULL) {
                while (i < tokens->token_len && tokens->tokens[i]->tokentype != TOK_EOL) {
                    i++;
                }
                if (i < tokens->token_len && tokens->tokens[i]->tokentype == TOK_EOL) {
                    discarded_tokens->tokens[discarded_tokens->token_len++] = tokens->tokens[i];
                    if (discarded_tokens->token_len >= discarded_tokens->token_cap) {
                        discarded_tokens->token_cap *= 2;
                        discarded_tokens->tokens = realloc(discarded_tokens->tokens, sizeof(pp_token_t*) * discarded_tokens->token_cap);
                    }

                    i++;
                }
                if (i < tokens->token_len && tokens->tokens[i]->tokentype == TOK_EOL) {
                    while (i < tokens->token_len && tokens->tokens[i]->tokentype != TOK_EOL) {
                        i++;
                    }
                }
                if (i < tokens->token_len && tokens->tokens[i]->tokentype == TOK_EOL) {
                    discarded_tokens->tokens[discarded_tokens->token_len++] = tokens->tokens[i];
                    if (discarded_tokens->token_len >= discarded_tokens->token_cap) {
                        discarded_tokens->token_cap *= 2;
                        discarded_tokens->tokens = realloc(discarded_tokens->tokens, sizeof(pp_token_t*) * discarded_tokens->token_cap);
                    }

                    i++;
                }
                i++;
                continue;
            }

            if (output_tokens->token_len + expanded_tokens->token_len > output_tokens->token_cap) {
                output_tokens->token_cap *= 2;
                output_tokens->tokens = realloc(output_tokens->tokens, sizeof(pp_token_t*) * output_tokens->token_cap);
            }

            for (size_t j = 0; j < expanded_tokens->token_len; j++) {
                output_tokens->tokens[output_tokens->token_len++] = expanded_tokens->tokens[j];
                output_tokens->token_len++;
            }
        }
        else{
            if (output_tokens->token_len >= output_tokens->token_cap) {
                output_tokens->token_cap *= 2;
                output_tokens->tokens = realloc(output_tokens->tokens, sizeof(pp_token_t*) * output_tokens->token_cap);
            }
            output_tokens->tokens[output_tokens->token_len++] = token;
            i++;
        }
    }

    pp_tok_list_free(discarded_tokens);
    free(metadata);

    pp_eval_ret_t* eval_ret = s_malloc(sizeof(pp_eval_ret_t));
    eval_ret->tokens = output_tokens;
    eval_ret->metadata = s_malloc(sizeof(pp_metadata_t));
    eval_ret->metadata->macro_metadata = macro_metadata;
    eval_ret->metadata->include_metadata = include_metadata;
    eval_ret->metadata->active_macros = macro_entries;
    return eval_ret;
}

pp_res_t* preprocessor_run(pp_context_t* context, diag_state_t* diag_state){
    pp_token_list_t* tokens = tokenize(context->filename, context->input, context->enable_trigraphs, context->include_stack, context->include_stack_len, diag_state);
    if (tokens == NULL){
        return NULL;
    }

    pp_eval_ret_t* eval_ret = pp_evaluate_tokens(tokens, context, diag_state);
    if (eval_ret == NULL){
        pp_tok_list_free(tokens);
        return NULL;
    }

    pp_res_t* result = s_malloc(sizeof(pp_res_t));
    result->metadata = eval_ret->metadata;
    result->output = pp_construct_output(eval_ret->tokens);

    pp_tok_list_free(eval_ret->tokens);

    return result;
}

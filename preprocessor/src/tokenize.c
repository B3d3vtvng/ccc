#include "../include/tokenize.h"

char* read_input(char* filename, char* input, diag_metadata_t* diag_metadata, diag_state_t* diag_state){
    if (input == NULL){
        FILE* fd = fopen(filename, "r");
        if (fd == NULL){
            char* errormsg;
            int len = snprintf(NULL, 0, "Fatal: Failed to open included file %s: %s", filename, strerror(errno));
            errormsg = s_malloc(sizeof(char) * len);
            snprintf(errormsg, len+1, "Fatal: Failed to open included file %s: %s", filename, strerror(errno));
            diag_add(diag_state, diag_metadata, PP_FATAL, errormsg, true, 0);
            return NULL;
        }

        char* input_buf;
        if (fseek(fd, 0, SEEK_END) != 0){
            goto read_fail;
        }

        long fsize = ftell(fd);
        if (fsize < 0){
            goto read_fail;
        }

        input_buf = s_malloc(sizeof(char) * fsize + 1);
        fread(input_buf, sizeof(char), fsize, fd);

        if (ferror(fd)){
            free(input_buf);
read_fail:  
            {char* errormsg;
            int len = snprintf(NULL, 0, "Fatal: Failed to read included file %s: %s", filename, strerror(errno));
            errormsg = s_malloc(sizeof(char) * len);
            snprintf(errormsg, len+1, "Fatal: Failed to read included file %s: %s", filename, strerror(errno));
            diag_add(diag_state, diag_metadata, PP_FATAL, errormsg, true, 0);
            fclose(fd);
            return NULL;}
        }

        fclose(fd);
        return input_buf;
    }
    else{
        return input;
    }
}

void substitute_trigraph(size_t line, size_t* col, size_t input_size, char* input, size_t* pos, tbuf_t* output, size_t* out_pos, diag_metadata_t* diag_metadata, diag_state_t* diag_state){
    if (*pos == input_size-2){
        diag_metadata->line = line;
        diag_metadata->col = *col+1;
        diag_add(diag_state, diag_metadata, PP_ERROR, "Invalid trigraph token at end of file: ??", false, 2);
        *col += 2;
        *pos += 2;
        return;
    }

    switch (input[*pos+2]){
        case '=': tracked_buffer_append(output, '#', line, *col); break;
        case '/': tracked_buffer_append(output, '\\', line, *col); break;
        case '(': tracked_buffer_append(output, '[', line, *col); break;
        case ')': tracked_buffer_append(output, ']', line, *col); break;
        case '!': tracked_buffer_append(output, '|', line, *col); break;
        case '<': tracked_buffer_append(output, '{', line, *col); break;
        case '>': tracked_buffer_append(output, '}', line, *col); break;
        case '-': tracked_buffer_append(output, '~', line, *col); break;
        case '\'': tracked_buffer_append(output, '^', line, *col); break;
        default:{
            diag_metadata->line = line;
            diag_metadata->col = *col+1;
            char* errormsg;
            int len = snprintf(NULL, 0, "Invalid trigraph token: ??%c", input[*pos+2]);
            errormsg = s_malloc(sizeof(char) * len);
            snprintf(errormsg, len+1, "Invalid trigraph token: ??%c", input[*pos+2]);
            diag_add(diag_state, diag_metadata, PP_ERROR, errormsg, true, 3);
            *pos += 2;
            *col += 2;
            (*out_pos)++;
            return;
        }
    }

    *pos += 3;
    *col += 3;
    (*out_pos)++;

    return;
}

tbuf_t* handle_trigraph_substitution(char* input, diag_metadata_t* diag_metadata, diag_state_t* diag_state){
    size_t input_size = strlen(input);
    tbuf_t* output = tracked_buffer_new();
    size_t line = 1, col = 0;
    size_t pos = 0;
    size_t out_pos = 0;
    while (pos <= input_size){
        if (input[pos] == '\n'){
            tracked_buffer_append(output, input[pos], line, col);
            line++; 
            col = 0; 
            pos++; 
            out_pos++;
        }
        if (input[pos] == '?' && pos <= input_size-2 && input[pos+1] == '?'){
            substitute_trigraph(line, &col, input_size, input, &pos, output, &out_pos, diag_metadata, diag_state);
            continue;
        }
        tracked_buffer_append(output, input[pos], line, col);
        pos++;
        out_pos++;
        col++;
    }

    return output;
}

line_buf_t* make_lines_c(char* input){
    line_buf_t* line_buf = s_malloc(sizeof(line_buf_t));
    line_buf->lines = s_malloc(sizeof(tbuf_t*) * 32);
    line_buf->cap = 32;
    line_buf->len = 0;
    tbuf_t* cur_tbuf = tracked_buffer_new();

    int i = 0;
    int line = 0;
    int col = 0;

    while (input[i] != '\0'){
        if (input[i] == '\\' && input[i+1] == '\n'){
            line++;
            col = 0;
            i += 2;
            continue;
        }

        tracked_buffer_append(cur_tbuf, input[i], line, col);

        if (input[i] == '\n'){
            line++;
            col = 0;
            if (line_buf->len >= line_buf->cap-1){
                line_buf->lines = s_realloc(line_buf->lines, line_buf->cap * 2);
                line_buf->cap *= 2;
            }
            line_buf->lines[line_buf->len] = cur_tbuf;
            cur_tbuf = tracked_buffer_new();
            line_buf->len++;
        }

        i++;
        col++;
    }

    return line_buf;
}

line_buf_t* make_lines_tb(tbuf_t* tb){
    line_buf_t* line_buf = s_malloc(sizeof(line_buf_t));
    line_buf->lines = s_malloc(sizeof(tbuf_t*) * 32);
    line_buf->cap = 32;
    line_buf->len = 0;
    tbuf_t* cur_tbuf = tracked_buffer_new();

    int i = 0;

    while (tb->buffer[i] != '\0'){
        if (tb->buffer[i] == '\\' && tb->buffer[i+1] == '\n'){
            i += 2;
            continue;
        }

        tracked_buffer_append(cur_tbuf, tb->buffer[i], tb->positions[i].line, tb->positions[i].column);

        if (tb->buffer[i] == '\n'){
            if (line_buf->len >= line_buf->cap-1){
                line_buf->lines = s_realloc(line_buf->lines, line_buf->cap * 2);
                line_buf->cap *= 2;
            }
            line_buf->lines[line_buf->len] = cur_tbuf;
            cur_tbuf = tracked_buffer_new();
            line_buf->len++;
        }

        i++;
    }

    return line_buf;
}

void tokenize_line(token_list_t* tokens, tbuf_t* line, diag_metadata_t* metadata, diag_state_t* diag_state){
    
}

token_list_t* tokenize(pp_tokenizer_context_t* context, diag_state_t* diag_state){
    diag_metadata_t* diag_metadata = s_malloc(sizeof(diag_metadata_t));
    diag_metadata->filename = context->filename;
    diag_metadata->include_stack = context->include_stack;
    diag_metadata->include_stack_len = context->include_stack_len;

    char* input = read_input(context->filename, context->input, diag_metadata, diag_state);
    if (input == NULL){
        return NULL;
    }

    line_buf_t* lines;

    if (context->enable_trigraphs){
        tbuf_t* subst_input = handle_trigraph_substitution(input, diag_metadata, diag_state);
        free(input);

        lines = make_lines_tb(subst_input);
        tracked_buffer_free(subst_input);
    }
    else{
        lines = make_lines_c(input);
        free(input);
    }

    token_list_t* tokens = s_malloc(sizeof(token_list_t));
    tokens->tokens = s_malloc(sizeof(token_t*) * 64);
    tokens->token_cap = 64;
    tokens->token_len = 0;

    for(int i = 0; i < lines->len; i++){
        tokenize_line(tokens, lines->lines[i], diag_metadata, diag_state);
    }

    return tokens;
}

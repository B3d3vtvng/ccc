#include "../include/tokenize.h"

const char* directives[] = {
    "include", 
    "define", 
    "undef", 
    "ifdef", 
    "ifndef", 
    "if", 
    "else", 
    "elif", 
    "error",
    "pragma",
    "line",
    "endif"
};

const char operator_start_chars[] = {
    '+', '-', '*', '/', '%',   // arithmetic
    '&', '|', '^', '~', '!',   // bitwise / logical
    '=', '<', '>',             // comparison / assignment
    '?', ':',                  // ternary
    ';', ',', '.',             // separators
    '#',                       // preprocessor and token-paste
    '(', ')', '[', ']', '{', '}', // grouping
    '\0' // Null terminate
};

const char *valid_operators[] = {
    // 3-char operators
    "<<=", ">>=", "...",

    // 2-char operators
    "++", "--", "==", "!=", "<=", ">=", "&&", "||",
    "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
    "<<", ">>", "->",

    // 1-char operators
    "+", "-", "*", "/", "%", "&", "|", "^", "~", "!",
    "=", "<", ">", "?", ":", ";", ",", ".",
    "(", ")", "[", "]", "{", "}",

    NULL
};



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

        fseek(fd, 0, SEEK_SET);

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
            errormsg = s_malloc(sizeof(char) * len+1);
            snprintf(errormsg, len, "Invalid trigraph token: ??%c", input[*pos+2]);
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
    while (pos < input_size){
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
    int input_len = strlen(input);

    int i = 0;
    int line = 1;
    int col = 0;

    while (i < input_len){
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
                line_buf->lines = s_realloc(line_buf->lines, line_buf->cap * 2 * sizeof(tbuf_t*));
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

    while (i < tb->length){
        if (tb->buffer[i] == '\\' && tb->buffer[i+1] == '\n'){
            i += 2;
            continue;
        }

        tracked_buffer_append(cur_tbuf, tb->buffer[i], tb->positions[i].line, tb->positions[i].column);

        if (tb->buffer[i] == '\n'){
            if (line_buf->len >= line_buf->cap-1){
                line_buf->lines = s_realloc(line_buf->lines, line_buf->cap * 2 * sizeof(tbuf_t*));
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

bool iswhitespace(char c){
    if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f') return true;
    return false;
}

uint32_t hex_to_uint32(char c) {
    if ('0' <= c && c <= '9') return c - '0';
    else if ('a' <= c && c <= 'f') return c - 'a' + 10;
    else if ('A' <= c && c <= 'F') return c - 'A' + 10;
    else return -1; // Invalid hex character
}

uint32_t parse_unicode_char(tbuf_t* line, int i, diag_state_t* diag_state, diag_metadata_t* metadata, size_t len){
    uint32_t val = 0;
    if (i < line->length-len/8){
        for (int j = 0; j < len/8; j++){
            uint32_t digit = hex_to_uint32(line->buffer[i+j]);
            if (digit == -1){
                metadata->line = line->positions[i+j].line;
                metadata->col = line->positions[i+j].column;
                size_t msg_len = snprintf(NULL, 0, "Invalid unicode literal: %c", line->buffer[i+j]);
                char* msg = s_malloc(sizeof(char) * msg_len + 1);
                snprintf(msg, msg_len, "Invalid unicode literal: %c", line->buffer[i+j]);
                diag_add(diag_state, metadata, PP_ERROR, msg, true, 1);
                return 0xffffffff;
            }

            val += digit << (j*4);
        }
    }
    else{
        return 0xffffffff;
    }

    return val;
}

bool pp_is_identifier_start(tbuf_t* line, int i, diag_state_t* diag_state, diag_metadata_t* metadata){
    uint32_t unicode_char;

    if (i < line->length-2 && line->buffer[i] == '\\' && line->buffer[i+1] == 'u'){
        unicode_char = parse_unicode_char(line, i+2, diag_state, metadata, 16);
    }
    else if (i < line->length-2 && line->buffer[i] == '\\' && line->buffer[i+1] == 'U'){
        unicode_char = parse_unicode_char(line, i+2, diag_state, metadata, 32);
    }
    else{
        unicode_char = line->buffer[i];
    }

    if (unicode_char == 0xffffffff) return false;

    return is_identifier_start(unicode_char);
}

bool pp_is_identifier_char(tbuf_t* line, int* i, diag_state_t* diag_state, diag_metadata_t* metadata){
    uint32_t unicode_char;

    if (*i < line->length-2 && line->buffer[*i] == '\\' && line->buffer[*i+1] == 'u'){
        unicode_char = parse_unicode_char(line, *i+2, diag_state, metadata, 16);
        *i += 6;
        if (unicode_char == 0xffffffff) return false;
    }
    else if (*i < line->length-2 && line->buffer[*i] == '\\' && line->buffer[*i+1] == 'U'){
        unicode_char = parse_unicode_char(line, *i+2, diag_state, metadata, 32);
        *i += 10;
        if (unicode_char == 0xffffffff) return false;
    }
    else{
        unicode_char = line->buffer[*i];
    }

    bool ret = is_identifier_char(unicode_char);
    if (ret) (*i)++;
    return ret;
}

int process_num(tbuf_t* line, int* i, diag_state_t* diag_state, diag_metadata_t* metadata){
    size_t j = 0;
    bool has_dot = false;
    int invalid = -1;
    while (*i+j < line->length-1){
        if (isdigit(line->buffer[*i+j])){
            j++; 
            continue;
        }
        else if (line->buffer[*i+j] == '.'){
            if (has_dot){
                invalid = true;
            }
            else has_dot = true;
            j++;
        }
        else break;
    }

    if (invalid != -1){
        metadata->line = line->positions[*i+invalid].line;
        metadata->col = line->positions[*i+invalid].column;
        char* invalid_token = strndup(&line->buffer[*i+invalid-1], j-invalid+1);
        size_t msg_len = snprintf(NULL, 0, "Invalid numerical literal: '%s'", invalid_token);
        char* msg = s_malloc(sizeof(char) * msg_len + 1);
        snprintf(msg, msg_len+1, "Invalid numerical literal: '%s'", invalid_token);
        free(invalid_token);
        diag_add(diag_state, metadata, PP_ERROR, msg, true, 1);
        *i += j;
        return -1;
    }
    *i += j;
    return 0;
}

bool is_directive(tbuf_t* line, int i){
    if (line->buffer[i] != '#') return false;
    i++;
    for (size_t j = 0; j < DIRECTIVE_COUNT; ++j) {
        size_t dlen = strlen(directives[j]);
        if (line->length-i-1 < dlen) continue;
        if (strncmp(&line->buffer[i], directives[j], dlen) == 0) return true;
    }
    return false;
}

void process_directive(tbuf_t* line, int* i){
    (*i)++;
    for (size_t j = 0; j < DIRECTIVE_COUNT; ++j) {
        size_t dlen = strlen(directives[j]);
        if (line->length-*i-j-1 < dlen) continue;
        if (strncmp(&line->buffer[*i], directives[j], dlen) == 0){
            *i += dlen;
            return;
        }
    }
    return; //This should never happen lmao
}

int process_char_lit(tbuf_t* line, int* i, diag_state_t* diag_state, diag_metadata_t* metadata){
    if (*i >= line->length-1){
        metadata->col = line->positions[*i-1].column;
        metadata->line = line->positions[*i-1].line;
        diag_add(diag_state, metadata, PP_ERROR, "Unterminated character literal", false, 2);
        return -1;
    }

    if (line->buffer[*i] == '\\'){
        if (*i >= line->length-3){
            metadata->col = line->positions[*i-1].column;
            metadata->line = line->positions[*i-1].line;
            diag_add(diag_state, metadata, PP_ERROR, "Unterminated character literal", false, 3);
            return -1;
        }

        *i += 2;

        if (line->buffer[*i] != '\''){
            metadata->col = line->positions[*i-2].column;
            metadata->line = line->positions[*i-2].line;
            diag_add(diag_state, metadata, PP_ERROR, "Unterminated character literal", false, 3);
            return -1;
        }
    }
    else{
        (*i)++;
        if (line->buffer[*i] != '\''){
            metadata->col = line->positions[*i-1].column;
            metadata->line = line->positions[*i-1].line;
            diag_add(diag_state, metadata, PP_ERROR, "Unterminated character literal", false, 2);
            return -1;
        }
    }

    return 0;
}

int process_str_lit(tbuf_t* line, int* i, diag_state_t* diag_state, diag_metadata_t* metadata){
    int start = (*i);
    while (*i < line->length-1){
        if (line->buffer[*i] == '"' && line->buffer[*i-1] != '\\'){
            return 0;
        }
        (*i)++;
    }

    metadata->col = line->positions[start].column;
    metadata->line = line->positions[start].line;
    diag_add(diag_state, metadata, PP_ERROR, "Unterminated string literal", false, *i-start);
    return -1;
}

int process_lit(tbuf_t* line, int* i, diag_state_t* diag_state, diag_metadata_t* metadata){
    char quote_char = line->buffer[*i];
    (*i)++;
    if (quote_char == '\''){
        return process_char_lit(line, i, diag_state, metadata);
    }
    else{
        return process_str_lit(line, i, diag_state, metadata);
    }
}

bool is_operator_start(char c){
    for (int i = 0; operator_start_chars[i] != '\0'; i++){
        if (c == operator_start_chars[i]) return true;
    }
    return false;
}

int process_operator(tbuf_t* line, int i){
    for (int j = 0; valid_operators[j]!= NULL; j++){
        size_t op_len = strlen(valid_operators[j]);
        if (i+op_len > line->length-1){
            continue;
        }
        if (strncmp(&line->buffer[i], valid_operators[j], op_len) == 0){
            return op_len;
        }
    }

    return -1; //should never happen
}

bool tokenize_line(pp_token_list_t* tokens, tbuf_t* line, diag_metadata_t* metadata, diag_state_t* diag_state, bool in_comment, src_pos_t** comment_start_pos){
    // Returns true if currently in multi-line comment
    int i = 0;
    bool inv_tok = false;
    int inv_tok_start;
    int inv_tok_len;

    if (in_comment){
        while (i < line->length-2 && !(line->buffer[i] == '*' && line->buffer[i+1] == '/')) i++;
        if (i == line->length-3) return true;
        pp_tok_append(tokens, TOK_EOL, strdup("\n"), line->positions[i], metadata->filename);
        i += 2;
    }

    while (i < line->length-1){
        if (in_comment){
            if (line->buffer[i] == '*' && i < line->length-2 && line->buffer[i+1] == '/'){
                in_comment = false;
                i += 2;
                continue;
            }
            else{
                i++;
                continue;
            }
        }
        if (iswhitespace(line->buffer[i])){
            int start = 0;
            while (iswhitespace(line->buffer[i+start])) start++;
            pp_tok_append(tokens, TOK_WHITESPACE, strndup(&line->buffer[i], start), line->positions[i], metadata->filename);
            i += start;
        }
        else if (pp_is_identifier_start(line, i, diag_state, metadata)){
            int start = i;
            while (pp_is_identifier_char(line, &i, diag_state, metadata)); //Read identifier chars until function returns false
            pp_tok_append(tokens, TOK_IDENTIFIER, strndup(&line->buffer[start], i-start), line->positions[start], metadata->filename);
        }
        else if (isdigit(line->buffer[i])){
            int start = i;
            if (process_num(line, &i, diag_state, metadata) == -1){
                continue;
            }
            pp_tok_append(tokens, TOK_NUMBER, strndup(&line->buffer[start], i-start), line->positions[start], metadata->filename);
        }
        else if (line->buffer[i] == '"' || line->buffer[i] == '\''){
            int start = i;
            char quote = line->buffer[i];
            if (process_lit(line, &i, diag_state, metadata) == -1){
                pp_tok_append(tokens, TOK_EOL, strdup("\n"), line->positions[line->length-1], metadata->filename);
                return false;
            }
            if (quote == '\''){
                pp_tok_append(tokens, TOK_CHAR_LIT, strndup(&line->buffer[start+1], i-start-1), line->positions[start], metadata->filename);
            }
            else{
                pp_tok_append(tokens, TOK_STRING_LIT, strndup(&line->buffer[start+1], i-start-1), line->positions[i], metadata->filename);
                i++;
            }
        }
        else if (is_directive(line, i)){
            int start = i;
            process_directive(line, &i);
            pp_tok_append(tokens, TOK_DIRECTIVE, strndup(&line->buffer[start], i-start), line->positions[start], metadata->filename);
        }
        else if (line->buffer[i] == '#'){
            if (i != line->length-2 && line->buffer[i+1] == '#'){
                pp_tok_append(tokens, TOK_DOUBLE_HASH, strndup(&line->buffer[i], 2), line->positions[i], metadata->filename);
                i += 2;
            }
            else{
                pp_tok_append(tokens, TOK_HASH, strndup(&line->buffer[i], 1), line->positions[i], metadata->filename);
                i++;
            }
        }
        else if (line->buffer[i] == '/' && i < line->length-2 && line->buffer[i+1] == '/'){
            pp_tok_append(tokens, TOK_WHITESPACE, strdup(" "), line->positions[i], metadata->filename);
            return false;
        }
        else if (line->buffer[i] == '/' && i < line->length-2 && line->buffer[i+1] == '*'){
            *comment_start_pos = &line->positions[i];
            return true;
        }
        else if (is_operator_start(line->buffer[i])){
            int op_len = process_operator(line, i);
            pp_tok_append(tokens, TOK_OPERATOR, strndup(&line->buffer[i], op_len), line->positions[i], metadata->filename);
            i += op_len;
        }
        else{
            if (!inv_tok){
                inv_tok = true;
                inv_tok_start = i;
                inv_tok_len = 1;
            }
            else{
                inv_tok_len++;
            }
            i++;
            continue;
        }

        if (inv_tok){
            metadata->line = line->positions[inv_tok_start].line;
            metadata->col = line->positions[inv_tok_start+1].column;
            char* invalid_token = strndup(&line->buffer[inv_tok_start], inv_tok_len);
            size_t err_msg_len = snprintf(NULL, 0, "Invalid token: '%s'", invalid_token);
            char* err_msg = s_malloc(sizeof(char) * err_msg_len + 1);
            snprintf(err_msg, err_msg_len+1, "Invalid token: '%s'", invalid_token);
            diag_add(diag_state, metadata, PP_ERROR, err_msg, true, strlen(invalid_token));

            inv_tok = false;
        }
    }

    if (inv_tok){
        metadata->line = line->positions[inv_tok_len].line;
        metadata->col = line->positions[inv_tok_len].column;
        char* invalid_token = strndup(&line->buffer[inv_tok_start], inv_tok_len);
        size_t err_msg_len = snprintf(NULL, 0, "Invalid token: %s", invalid_token);
        char* err_msg = s_malloc(sizeof(char) * err_msg_len + 1);
        snprintf(err_msg, err_msg_len, "Invalid token: %s", invalid_token);
        diag_add(diag_state, metadata, PP_ERROR, err_msg, true, strlen(invalid_token));
    }

    pp_tok_append(tokens, TOK_EOL, strdup("\n"), line->positions[line->length-1], metadata->filename);
    return false;
}

int get_comment_len(line_buf_t* lines, src_pos_t* start_pos){
    int len = 0;
    for (int i = 0; i < lines->len; i++){
        tbuf_t* line = lines->lines[i];
        if (line->positions[0].line == start_pos->line){
            len += line->positions[line->length-1].column - start_pos->column + 1; // +1 for the newline character
        }
        else if (line->positions[0].line > start_pos->line){
            len += line->length; // Full line length
        }
    }
    return len;
}

pp_token_list_t* tokenize(char* filename, char* input, bool enable_trigraphs, char** include_stack, size_t include_stack_len, diag_state_t* diag_state){
    diag_metadata_t* diag_metadata = s_malloc(sizeof(diag_metadata_t));
    diag_metadata->filename = filename;
    diag_metadata->include_stack = include_stack;
    diag_metadata->include_stack_len = include_stack_len;

    char* new_input = read_input(filename, input, diag_metadata, diag_state);
    if (new_input == NULL){
        return NULL;
    }

    line_buf_t* lines;

    if (enable_trigraphs){
        tbuf_t* tbuf_input = handle_trigraph_substitution(input, diag_metadata, diag_state);
        free(input);

        lines = make_lines_tb(tbuf_input);
        tracked_buffer_free(tbuf_input);
    }
    else{
        lines = make_lines_c(input);
        free(input);
    }

    pp_token_list_t* tokens = s_malloc(sizeof(pp_token_list_t));
    tokens->tokens = s_malloc(sizeof(pp_token_t*) * 64);
    tokens->token_cap = 64;
    tokens->token_len = 0;

    bool in_comment = false;
    src_pos_t* comment_start_pos;

    for(int i = 0; i < lines->len; i++){
        in_comment = tokenize_line(tokens, lines->lines[i], diag_metadata, diag_state, in_comment, &comment_start_pos);
    }

    if (in_comment){
        diag_metadata->line = comment_start_pos->line;
        diag_metadata->col = comment_start_pos->column+1;
        diag_add(diag_state, diag_metadata, PP_ERROR, "Unterminated multi-line comment", false, get_comment_len(lines, comment_start_pos)-1);
    }

    src_pos_t last_pos;
    if (lines->len == 0){
        last_pos = (src_pos_t){0, 0};
    }
    else{
        tbuf_t* last_line = lines->lines[lines->len-1];
        src_pos_t last_pos = last_line->positions[last_line->length-1];
        last_pos.column++;
    }

    pp_tok_append(tokens, TOK_EOF, NULL, last_pos, filename);

    for (int i = 0; i < lines->len; i++){
        tracked_buffer_free(lines->lines[i]);
    }
    free(lines->lines);
    free(lines);

    free(diag_metadata);

    return tokens;
}

#ifndef PP_TOKEN_H
#define PP_TOKEN_H

#include <string.h>
#include "../../include/tbuf.h"

typedef enum{
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING_LIT,
    TOK_CHAR_LIT,
    TOK_OPERATOR,
    TOK_PUNCT,
    TOK_DIRECTIVE,
    TOK_HASH,
    TOK_DOUBLE_HASH,
    TOK_EOL,
    TOK_WHITESPACE,
    TOK_EOF,
} pp_tokentype_t;

typedef struct{
    pp_tokentype_t tokentype;
    char* value;
    src_pos_t src_pos;
    char* filename;
} pp_token_t;

typedef struct{
    pp_token_t** tokens;
    size_t token_len;
    size_t token_cap;
} pp_token_list_t;

void pp_tok_free(pp_token_t* tok){
    if (tok->value != NULL){
        free(tok->value);
    }
    free(tok);
    return;
}

void pp_tok_list_free(pp_token_list_t* tok_list){
    for (int i = 0; i < tok_list->token_len; i++){
        pp_tok_free(tok_list->tokens[i]);
    }

    free(tok_list);
    return;
}

void pp_tok_append(pp_token_list_t* tok_list, pp_tokentype_t tokentype, char* value, src_pos_t pos, char* filename){
    pp_token_t* new_token = s_malloc(sizeof(pp_token_t));
    new_token->tokentype = tokentype;
    new_token->value = value;
    new_token->src_pos = pos;
    new_token->filename = filename;

    if (tok_list->token_len >= tok_list->token_cap-1){
        tok_list->tokens = s_realloc(tok_list->tokens, tok_list->token_cap*2*sizeof(pp_token_t*));
        tok_list->token_cap *= 2;
    }

    tok_list->tokens[tok_list->token_len] = new_token;
    tok_list->token_len++;

    return;
}

char* pp_tok_type_str(pp_tokentype_t t) {
    switch (t) {
        case TOK_IDENTIFIER: return "IDENT";
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING_LIT: return "STRING";
        case TOK_CHAR_LIT: return "CHAR";
        case TOK_OPERATOR: return "OP";
        case TOK_PUNCT: return "PUNCT";
        case TOK_DIRECTIVE: return "DIRECTIVE";
        case TOK_HASH: return "HASH";
        case TOK_DOUBLE_HASH: return "DOUBLE_HASH";
        case TOK_EOL: return "EOL";
        case TOK_WHITESPACE: return "WS";
        case TOK_EOF: return "EOF";
        default: return "???";
    }
}

void pp_tok_display(pp_token_t* tok){
    char* tok_type_str = pp_tok_type_str(tok->tokentype);
    printf("%s: \"%s\"", tok_type_str, tok->value);
    return;
}

void pp_tok_list_display(pp_token_list_t* tok_list){
    if (tok_list == NULL) return;
    if (tok_list->token_len == 0){
        printf("%s\n", "[]");
        return;
    }

    putc('[', stdout);
    for (int i = 0; i < tok_list->token_len; i++){
        pp_tok_display(tok_list->tokens[i]);
        if (i != tok_list->token_len-1){
            printf("%s", ", ");
        }
    }
    puts("]\n");
    return;
}

#endif

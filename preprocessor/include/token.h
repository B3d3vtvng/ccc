#ifndef PP_TOKEN_H
#define PP_TOKEN_H

#include <string.h>

typedef enum{
    TOK_IDENTIFIER,
    TOK_PP_NUMBER,
    TOK_STRING_LITERAL,
    TOK_CHAR_CONST,
    TOK_HEADER_NAME,
    TOK_OPERATOR,
    TOK_DIRECTIVE,
    TOK_HASH,
    TOK_DOUBLE_HASH,
    TOK_NEWLINE,
    TOK_WHITESPACE,
    TOK_EOF,
    TOK_UNKNOWN
} tokentype_t;

typedef struct{
    tokentype_t  tokentype;
    char* value;
    size_t valuelen;
    int line, col;
    char* filename;
} token_t;

typedef struct{
    token_t** tokens;
    size_t token_len;
    size_t token_cap;
} token_list_t;

#endif

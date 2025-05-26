#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "token.h"
#include "../../include/error.h"
#include "../../include/general.h"
#include "../../include/tbuf.h"
#include "../../include/unicode.h"

#define DIRECTIVE_COUNT 12

pp_token_list_t* tokenize(char* filename, char* input, bool enable_trigraphs, char** include_stack, size_t include_stack_len, diag_state_t* diag_state);

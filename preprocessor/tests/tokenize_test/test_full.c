#include "../../src/tokenize.c"

int main(void){
    diag_state_t* diag_state = diag_init();
    
    pp_token_list_t* tokens = tokenize("../../include/tokenize.h", NULL, false, NULL, 0, diag_state);

    if (tokens){
        pp_tok_list_display(tokens);
    }
    diag_display(diag_state);
    diag_free(diag_state);
    pp_tok_list_free(tokens);
    return 0;
}

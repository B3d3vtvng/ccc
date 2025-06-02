#include "../../include/tbuf.h"
#include "../../include/error.h"
#include "metadata.h"
#include "../../include/general.h"
#include "../include/token.h"
#include "../include/tokenize.h"

pp_res_t* preprocesser_run(pp_context_t* context, diag_state_t* diag_state);
pp_eval_ret_t* pp_evaluate_tokens(pp_token_list_t* tokens, pp_context_t* context, diag_state_t* diag_state);

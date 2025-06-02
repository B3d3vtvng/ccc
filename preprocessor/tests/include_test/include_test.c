#include "../../src/tokenize.c"
#include "../../src/preprocessor.c"

int main(void){
    pp_context_t* context = malloc(sizeof(pp_context_t));
    context->enable_trigraphs = false;
    context->filename = "tests/include_test/test.c";
    context->include_path_count = 2;
    context->include_paths = malloc(sizeof(pp_include_path_t**) * 10);
    context->include_paths[0] = malloc(sizeof(pp_include_path_t*));
    context->include_paths[0]->path = "tests/include_test";
    context->include_paths[0]->type = PP_INCL_LOCAL;
    context->include_paths[1] = malloc(sizeof(pp_include_path_t*));
    context->include_paths[1]->path = "../stdlib/include";
    context->include_paths[1]->type = PP_INCL_SYSTEM;
    context->include_path_cap = 10;
    context->include_stack = malloc(sizeof(char*) * 8);
    context->include_stack_len = 0;
    context->include_stack_cap = 8;
    context->input = NULL;

    diag_state_t* diag_state = diag_init();

    pp_res_t* res = preprocessor_run(context, diag_state);
    diag_display(diag_state);
    if (res != NULL){
        printf("%s\n", res->output->buffer);
        tracked_buffer_free(res->output);
    }
    diag_free(diag_state);
    free(context->include_paths);
    free(context->include_stack);
    free(context);

    return 0;
}

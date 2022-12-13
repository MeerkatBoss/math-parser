#include <stdio.h>
#include <string.h>

#include "logger.h"

#include "diff_utils.h"

int prog_init(prog_state *state, const char *filename)
{
    FILE* input = fopen(filename, "r");
    LOG_ASSERT_ERROR(input != NULL, return -1, "File not found '%s'", filename);

    LOG_ASSERT_ERROR(
        fscanf(input, " $f(%m[^)]) = %m[^$]$", &state->var_name, &state->function) == 2,
        return -1,
        "Corrupted input file '%s': invalid function", filename
    );
    char* read_var = NULL;
    LOG_ASSERT_ERROR(
        fscanf(input, " Taylor series at %lg to $%m[^^]^%d$",
            &state->taylor_at,
            &read_var,
            &state->taylor_pow) == 3,
        return -1,
        "Corrupted input file '%s': invalid Taylor series", filename
    );
    LOG_ASSERT_ERROR(
        strcmp(read_var, state->var_name) == 0,
        {free(read_var); return -1;},
        "Corrupted input file '%s': function variable '%s' does not match with Taylor series variable '%s'",
        filename, state->var_name, read_var
    );
    free(read_var);
    LOG_ASSERT_ERROR(
        fscanf(input, " Tangent at $%m[^=]=%lg$", &read_var, &state->tangent_at) == 2,
        return -1,
        "Corrupted input file '%s': invalid tangent", filename
    );
    LOG_ASSERT_ERROR(
        strcmp(read_var, state->var_name) == 0,
        {free(read_var); return -1;},
        "Corrupted input file '%s': function variable '%s' does not match with tangent variable '%s'",
        filename, state->var_name, read_var
    );
    free(read_var);
    LOG_ASSERT_ERROR(
        fscanf(input, " Plot in range [%lg, %lg]", &state->range_start, &state->range_end) == 2,
        return -1,
        "Corrupted input file '%s': invalid range",
        filename
    );
    LOG_ASSERT_ERROR(
        state->range_start < state->range_end,
        return -1,
        "Corrupted input file '%s': invalid range boundaries",
        filename
    );
    fclose(input);
    return 0;
}

void prog_state_dtor(prog_state *state)
{
    free(state->function);
    free(state->var_name);
    memset(state, 0, sizeof(*state));
}

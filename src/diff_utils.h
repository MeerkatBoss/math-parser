#ifndef DIFF_UTILS_H
#define DIFF_UTILS_H

struct prog_state
{
    char* function;
    char* var_name;
    double taylor_at;
    int taylor_pow;
    double tangent_at;
    double range_start;
    double range_end;
};

int prog_init(prog_state* state, const char* filename);
void prog_state_dtor(prog_state* state);

#endif
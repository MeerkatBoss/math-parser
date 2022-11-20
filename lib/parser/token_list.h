#ifndef LIST_STRING_H
#define LIST_STRING_H

#include <stddef.h>

#define COS(x) cos(x)

#define MATH_FUNC(name, ...) TOK_##name,

enum token_type
{
    TOK_NONE,
    TOK_NUM,
    TOK_PLUS,
    TOK_MINUS,
    TOK_CDOT,
    TOK_FRAC,
    TOK_CARET,
    TOK_PARENL,
    TOK_PARENR,
    TOK_VAR,
    #include "functions.h"
}

#undef MATH_FUNC

struct token
{
    token_type type;
    union {
        const char* name;
        double num;
    } value;
};

#define REDEFINE_ELEMENT
typedef token element_t;
const element_t POISON = {.type = TOK_NONE};
#include "list.h"

#endif
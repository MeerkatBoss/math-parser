#ifndef TOKEN_LIST_H
#define TOKEN_LIST_H

#include <stddef.h>

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
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_VAR,
    #include "functions.h"
};

#undef MATH_FUNC

const size_t MAX_NAME = 32;

struct token
{
    token_type type;
    union {
        char* name;
        double num;
    } value;
};

#define REDEFINE_ELEMENT
typedef token element_t;
const element_t POISON = {.type = TOK_NONE};
#include "list.h"

#endif
#ifndef TOKEN_ARRAY_H
#define TOKEN_ARRAY_H

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

// TODO: move closer, undef!
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
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_VAR,
    #include "functions.h"
    TOK_EOF
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

#define ARRAY_ELEMENT token

/* Shallow copy */
inline void copy_element(ARRAY_ELEMENT* dest, const ARRAY_ELEMENT* src)
{
    dest->type = src->type;
    dest->value = src->value;
}

inline void delete_element(ARRAY_ELEMENT* element)
{
    if (element->type == TOK_VAR)
        free(element->value.name);

    element = {};
}

#include "dynamic_array.h"

#undef ARRAY_ELEMENT

#endif
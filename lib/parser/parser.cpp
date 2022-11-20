#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "logger.h"

#include "parser.h"

static int strpref(const char* str1, const char* str2);

compact_list* parse_tokens(const char* str)
{
    compact_list *tokens = (compact_list*)calloc(1, sizeof(*tokens));
    *tokens = list_ctor();

    while(*str)
    {
        while (*str && isspace(*str))
            str++;

        if(!*str) break;

        double number = 0;
        int n_read = 0;
        
        if(sscanf(str, " %lg %n", &number, &n_read) == 1)
        {
            push_back(tokens, {.type = TOK_NUM, .value = {.num = number}});
            str += n_read;
            continue;
        }
        
        switch (*(str++))
        {
        case '+': push_back(tokens, {.type = TOK_PLUS});   continue;
        case '-': push_back(tokens, {.type = TOK_MINUS});  continue;
        case '^': push_back(tokens, {.type = TOK_CARET});  continue;
        case '(': push_back(tokens, {.type = TOK_LPAREN}); continue;
        case ')': push_back(tokens, {.type = TOK_RPAREN}); continue;
        case '{': push_back(tokens, {.type = TOK_LBRACE}); continue;
        case '}': push_back(tokens, {.type = TOK_RBRACE}); continue;

        default:
            str--;
            break;
        }

        #define MATH_FUNC(name, ...)                        \
            if(strpref(str, "\\"#name))                     \
            {                                               \
                str += sizeof("\\"#name)/sizeof(char) - 1;  \
                push_back(tokens, {.type = TOK_##name});    \
                continue;                                   \
            }

        MATH_FUNC(CDOT)
        MATH_FUNC(FRAC)
        #include "functions.h"

        #undef MATH_FUNC

        token tok = {.type = TOK_VAR, .value = {.name = NULL}};        

        LOG_ASSERT_ERROR(
            sscanf(str, " %32m[\\A-Za-z_] %n", &tok.value.name, &n_read) == 1,
            {
                list_dtor(tokens);
                return NULL;
            },
            "Invalid symbol: '%c'", *str
        );

        push_back(tokens, tok);
        str += n_read;
    }

    return tokens;
}

int strpref(const char *str, const char *pref)
{
    return strncasecmp(str, pref, strlen(pref)) == 0;
}

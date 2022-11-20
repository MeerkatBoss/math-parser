#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"

static int strpref(const char* str1, const char* str2);

compact_list* parse_tokens(const char* str)
{
    compact_list *tokens = (compact_list*)calloc(1, sizeof(*tokens));
    list_ctor(tokens);

    while(*str)
    {
        while (isspace(*str))
            str++;
        
        switch (*++str)
        {
        case '+': push_back(tokens, {.type = TOK_PLUS});   continue;
        case '-': push_back(tokens, {.type = TOK_MINUS});  continue;
        case '(': push_back(tokens, {.type = TOK_LPAREN}); continue;
        case ')': push_back(tokens, {.type = TOK_RPAREN}); continue;
        case '{': push_back(tokens, {.type = TOK_LBRACE}); continue;
        case '}': push_back(tokens, {.type = TOK_RBRACE}); continue;

        default:
            str--;
            break;
        }

        #define MATH_FUNC(name, ...)                    \
            if(strpref(str, "\\"#name))                 \
            {                                           \
                str += sizeof("\\"#name)/sizeof(char);  \
                push_back(tokens, {.type = TOK_##name});\
                continue;                               \
            }

        MATH_FUNC(CDOT)
        MATH_FUNC(FRAC)
        #include "functions.h"

        #undef MATH_FUNC
    }

    return tokens;
}

int strpref(const char *str, const char *pref)
{
    return strncasecmp(str, pref, strlen(pref)) == 0;
}

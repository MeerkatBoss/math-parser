#include <stdio.h>
#include <stdlib.h>

#include "logger.h"

#include "parser.h"

int main()
{
    add_default_file_logger();

    #define MATH_FUNC(name, ...) #name,

    const char *names[] = {
        "NONE",
        "NUM",
        "PLUS",
        "MINUS",
        "CDOT",
        "FRAC",
        "CARET",
        "LPAREN",
        "RPAREN",
        "LBRACE",
        "RBRACE",
        "VAR",
        #include "functions.h"
    };

    #undef MATH_FUNC
    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch-enum"       

    compact_list* tokens = parse_tokens("\\frac{1}{x^2 - 2\\cos x + 1}");
    for (list_iterator i = list_begin(tokens); i; i = next_element(tokens, i))
    {
        token tok = get_element(tokens, i);
        switch (tok.type)
        {
        case TOK_NUM:
            printf("num: %g\n", tok.value.num);
            break;
        case TOK_VAR:
            printf("var: \"%s\"\n", tok.value.name);
            break;
        default:
            printf("%s\n", names[tok.type]);
            break;
        }
    }

    #pragma GCC diagnostic pop

    putc('\n', stdout);
    for (list_iterator i = list_begin(tokens); i; i = next_element(tokens, i))
        if (get_element(tokens, i).type == TOK_VAR)
            free(get_element(tokens, i).value.name);
    list_dtor(tokens);
    free(tokens);
    return 0;
}
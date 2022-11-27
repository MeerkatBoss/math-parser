#include <stdio.h>
#include <stdlib.h>

#include "logger.h"

#include "lexer.h"
#include "parser.h"
#include "tree_math.h"

int main()
{
    add_default_file_logger();
    add_logger({
        .name = "Console Logger",
        .stream = stdout,
        .logging_level = LOG_ERROR,
        .settings_mask = LGS_KEEP_OPEN | LGS_USE_ESCAPE
    });

    #define MATH_FUNC(name, ...) #name,

    const char *names[] = {
        "+",
        "-",
        "*",
        "/",
        "^",
        "-",
        #include "functions.h"
    };

    #undef MATH_FUNC
    

    compact_list* tokens = parse_tokens("x^2 + 2 \\cdot x - 1");
    syntax_tree* ast = build_tree(tokens);
    syntax_tree* deriv = derivative(ast, "x");
    simplify(deriv);

    for (ast_node* node = tree_begin(deriv); node != NULL; node = next_iterator(node))
        switch(node->type)
        {
            case T_NUM:
                printf("%g ", node->value.num);
                break;
            case T_VAR:
                printf("%s ", ast->vars[node->value.var_id]);
                break;
            case T_OP:
                printf("%s ", names[node->value.op]);
                break;
            default:
                printf("[ERROR NODE] ");
                break;
        }
    printf("\nPrint ended\n");

    for (list_iterator it = list_begin(tokens);
            it != 0;
            it = next_element(tokens, it))
        if (get_element(tokens, it)->type == TOK_VAR)
            free(get_element(tokens, it)->value.name);

    list_dtor(tokens);
    free(tokens);
    tree_dtor(ast);
    tree_dtor(deriv);
    return 0;
}
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

    compact_list* tokens = parse_tokens("(\\sin x + \\arctan x)^{\\frac{42 - \\sqrt{x^2 - 3}}{4 - x^4}}");
    syntax_tree* ast = build_tree(tokens);
    syntax_tree* deriv = derivative(ast, "x");
    simplify(deriv);

    print_tree(ast, stdout);
    putc('\n', stdout);
    print_tree(deriv, stdout);
    putc('\n', stdout);

    for (list_iterator it = list_begin(tokens);
            it != 0;
            it = next_element(tokens, it))
    {
        if (get_element(tokens, it)->type == TOK_VAR)
            free(get_element(tokens, it)->value.name);
    }

    list_dtor(tokens);
    free(tokens);
    tree_dtor(ast);
    tree_dtor(deriv);
    return 0;
}
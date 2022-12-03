#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    }); // TODO: can this be in default logger?

    dynamic_array(token)* tokens = parse_tokens("(x + 1)^{\\frac{\\sin x}{2}} \\cdot (\\arctan \\sqrt{x^2 + 1})^{x - 2}");
    // dynamic_array(token)* tokens = parse_tokens("\\frac{1}{x + 1}");
    abstract_syntax_tree* ast = build_tree(tokens);
    abstract_syntax_tree* deriv = derivative(ast, "x");
    abstract_syntax_tree* maclaurin = maclaurin_series(ast, "x", 3);

    string_builder builder = {};
    string_builder_ctor(&builder);

    print_node(ast->root, &builder);
    print_node(deriv->root, &builder);
    print_node(maclaurin->root, &builder);

    string_builder_write(&builder, STDOUT_FILENO);

    array_dtor(tokens);
    free(tokens);

    tree_dtor(ast);
    tree_dtor(deriv);
    tree_dtor(maclaurin);
    string_builder_dtor(&builder);
    return 0;
}
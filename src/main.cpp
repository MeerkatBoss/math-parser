#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "logger.h"

#include "lexer.h"
#include "parser.h"
#include "tree_math.h"
#include "article_builder.h"

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
    abstract_syntax_tree* ast = build_tree(tokens);

    article_builder article = {};
    article_ctor(&article);
    article_use_preamble(&article, "assets/preamble.sty");
    article_use_starters(&article, "assets/starters.txt");
    article_use_transitions(&article, "assets/transitions.txt");
    article_use_placeholders(&article, "assets/placeholders.txt");

    article_start(&article);
    article_add_abstract(&article, "Wonderful article");

    article_add_section(&article, "Derivative");
    abstract_syntax_tree* deriv = derivative(ast, "x", &article);

    article_add_section(&article, "Taylor series");
    abstract_syntax_tree* taylor = taylor_series(ast, 0, "x", 3, &article);

    article_add_section(&article, "Tangent");
    abstract_syntax_tree* tangent = taylor_series(ast, 5, "x", 1, &article);

    plot_tangent(ast->root, tangent->root, "output/plot.png");
    string_builder_append(&article.text, "\\includegraphics{\"plot.png\"}\n");

    article_end(&article);

    article_build(&article, "output");

    array_dtor(tokens); free(tokens);
    tree_dtor(ast);
    tree_dtor(deriv);
    tree_dtor(taylor);
    tree_dtor(tangent);
    article_dtor(&article);
    return 0;
}
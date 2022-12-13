#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "logger.h"

#include "lexer.h"
#include "parser.h"
#include "tree_math.h"
#include "article_builder.h"

#include "diff_utils.h"

int main(int argc, const char** argv)
{
    add_default_file_logger();
    add_logger({
        .name = "Console Logger",
        .stream = stdout,
        .logging_level = LOG_ERROR,
        .settings_mask = LGS_KEEP_OPEN | LGS_USE_ESCAPE
    }); // TODO: can this be in default logger?

    prog_state state = {};
    const char* filename = argc > 1 
                            ? argv[1]
                            : "Funcfile";

    LOG_ASSERT(prog_init(&state, filename) == 0, {prog_state_dtor(&state); return 1;});

    dynamic_array(token)* tokens = parse_tokens(state.function);
    LOG_ASSERT(tokens != NULL, {prog_state_dtor(&state); return 1;});
    abstract_syntax_tree* ast = build_tree(tokens);
    array_dtor(tokens); free(tokens);
    LOG_ASSERT(ast != NULL, {prog_state_dtor(&state); return 1;});

    article_builder article = {};
    article_ctor(&article);
    article_use_preamble(&article, "assets/preamble.sty");
    article_use_starters(&article, "assets/starters.txt");
    article_use_transitions(&article, "assets/transitions.txt");
    article_use_placeholders(&article, "assets/placeholders.txt");

    article_start(&article);
    article_add_abstract(&article, "Wonderful article");

    article_add_section(&article, "Derivative");
    abstract_syntax_tree* deriv = derivative(ast, state.var_name, &article);

    article_add_section(&article, "Taylor series");
    abstract_syntax_tree* taylor = taylor_series(ast, state.taylor_at, state.var_name, state.taylor_pow, &article);

    article_add_section(&article, "Tangent");
    abstract_syntax_tree* tangent = taylor_series(ast, state.tangent_at, state.var_name, 1, &article);

    plot_tangent(ast->root, tangent->root, "output/plot.png", state.range_start, state.range_end);
    string_builder_append(&article.text, "\\includegraphics{\"plot.png\"}\n");

    article_end(&article);

    article_build(&article, "output");

    tree_dtor(ast);
    tree_dtor(deriv);
    tree_dtor(taylor);
    tree_dtor(tangent);
    article_dtor(&article);
    prog_state_dtor(&state);
    return 0;
}
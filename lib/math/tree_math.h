#ifndef TREE_MATH_H
#define TREE_MATH_H

#include <stddef.h>

#include "article_builder.h"

#include "ast.h"

// TODO: make docs
abstract_syntax_tree* derivative(abstract_syntax_tree* ast, const char* var, article_builder* article);

void simplify(abstract_syntax_tree* ast);

abstract_syntax_tree* maclaurin_series(abstract_syntax_tree* ast, const char* var, int pow, article_builder* article);

#endif
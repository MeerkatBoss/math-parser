#ifndef TREE_MATH_H
#define TREE_MATH_H

#include "ast.h"


// TODO: make docs
abstract_syntax_tree* derivative(abstract_syntax_tree* ast, const char* var);

void simplify(abstract_syntax_tree* ast);

abstract_syntax_tree* maclaurin_series(abstract_syntax_tree* ast, const char* var, int pow);

#endif
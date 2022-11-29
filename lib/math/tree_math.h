#ifndef TREE_MATH_H
#define TREE_MATH_H

#include "ast.h"


// TODO: make docs
syntax_tree* derivative(syntax_tree* ast, const char* var);

void simplify(syntax_tree* ast);

syntax_tree* maclaurin_series(syntax_tree* ast, const char* var, int pow);

#endif
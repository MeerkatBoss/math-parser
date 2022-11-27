#ifndef TREE_MATH_H
#define TREE_MATH_H

#include "ast.h"

syntax_tree* derivative(syntax_tree* ast, const char* var);

void simplify(syntax_tree* ast);

#endif
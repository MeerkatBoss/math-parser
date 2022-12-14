#ifndef PARSER_H
#define PARSER_H

#include "token_array.h"

#include "ast.h"

/**
 * @brief Build abstract syntax tree from list of tokens
 * @param[in] tokens Token list
 * @return Built AST
 */
abstract_syntax_tree* build_tree(const dynamic_array(token)* tokens);

#endif
#ifndef LEXER_H
#define LEXER_H

#include "token_array.h"

/**
 * @brief 
 * Split input string into separate lexemes
 * 
 * @param[in] str Input string
 * @return list of tokens
 */
dynamic_array(token)* parse_tokens(const char* str);

#endif
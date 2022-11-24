#ifndef LEXER_H
#define LEXER_H

#include "token_list.h"

/**
 * @brief 
 * Split input string into separate lexemes
 * 
 * @param[in] str Input string
 * @return list of tokens
 */
compact_list* parse_tokens(const char* str);

#endif
#ifndef PARSER_H
#define PARSER_H

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
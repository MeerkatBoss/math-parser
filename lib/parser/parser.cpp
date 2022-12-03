#include <string.h>

#include "logger.h"

#include "parser.h"

struct parsing_state
{
    const dynamic_array(token)* tokens;
    size_t pos;
    dynamic_array(var_name)* variables;
};

static ast_node* parse_expression(parsing_state* state);
static ast_node* parse_product(parsing_state* state);
static ast_node* parse_unary(parsing_state* state);
static ast_node* parse_fraction(parsing_state* state);
static ast_node* parse_power(parsing_state* state);
static ast_node* parse_group(parsing_state* state);
static ast_node* parse_atom(parsing_state* state);

static inline token* current_token(parsing_state* state)
{
    return array_get_element(state->tokens, state->pos);
}

static inline token* last_token(parsing_state* state)
{
    return array_get_element(state->tokens, state->pos - 1);
}

static inline void advance(parsing_state* state)
{
    state->pos++;
}

static inline token* consume(parsing_state* state, token_type expected)
{
    token* cur = current_token(state);
    if (cur->type == expected)
        advance(state);
    return cur;
}

static inline int consume_check(parsing_state* state, token_type expected)
{
    return consume(state, expected)->type == expected;
}

abstract_syntax_tree* build_tree(const dynamic_array(token)* tokens)
{
    abstract_syntax_tree* ast = tree_ctor();
    parsing_state state = {
        .tokens    = tokens,
        .pos       = 0,
        .variables = &ast->variables
    };
    ast->root = parse_expression(&state);
    LOG_ASSERT(ast->root,
    {
        tree_dtor(ast);
        return NULL;
    });
    LOG_ASSERT_ERROR(consume_check(&state, TOK_EOF),
    {
        tree_dtor(ast);
        return NULL;
    }, "Unexpected tokens after expression end.", NULL);
    return ast;
}

static ast_node* parse_expression(parsing_state* state)
{
    ast_node* left = parse_product(state);
    while(consume_check(state, TOK_PLUS) || consume_check(state, TOK_MINUS))
    {
        token* tok = last_token(state);
        ast_node* right = parse_product(state);
        LOG_ASSERT(right != NULL, return NULL);

        if (tok->type == TOK_PLUS)
            left = make_binary_node(OP_ADD, left, right);
        else
            left = make_binary_node(OP_SUB, left, right);
    }
    return left;
}

static ast_node * parse_product(parsing_state * state)
{
    ast_node* left = parse_unary(state);
    while(consume_check(state, TOK_CDOT))   /* TODO: handle implicit multiplication */
    {
        ast_node* right = parse_unary(state);
        LOG_ASSERT(right != NULL, return NULL);
        left = make_binary_node(OP_MUL, left, right);
    }
    return left;
}

ast_node * parse_unary(parsing_state * state)
{
    #define MATH_FUNC(name, ...)                                    \
        case TOK_##name:                                            \
            advance(state);                                         \
            return make_unary_node(OP_##name, parse_unary(state));

    switch (current_token(state)->type)
    {
    case TOK_MINUS:
        advance(state);
        return make_unary_node(OP_NEG, parse_unary(state));

    #include "functions.h"
    
    default:
        return parse_fraction(state);
    }

    LOG_ASSERT(0 && "Unreachable code", return NULL);
}

ast_node * parse_fraction(parsing_state * state)
{
    if (!consume_check(state, TOK_FRAC))
        return parse_power(state);

    LOG_ASSERT_ERROR(consume_check(state, TOK_LBRACKET), return NULL,
        "Expected '{' after '\\frac'", NULL);
    
    ast_node* left = parse_expression(state);

    LOG_ASSERT_ERROR(consume_check(state, TOK_RBRACKET),
        {delete_subtree(left); return NULL;},
        "Expected '}' after first argument of '\\frac'", NULL);

    LOG_ASSERT_ERROR(consume_check(state, TOK_LBRACKET), 
        {delete_subtree(left); return NULL;},
        "Expected '{' before second argument of '\\frac'", NULL);

    ast_node* right = parse_expression(state);

    LOG_ASSERT_ERROR(consume_check(state, TOK_RBRACKET),
        {delete_subtree(left); delete_subtree(right); return NULL;},
        "Expected '}' after second argument of '\\frac'", NULL);

    return make_binary_node(OP_DIV, left, right);
}

ast_node * parse_power(parsing_state * state)
{
    ast_node* left = parse_group(state);

    if (consume_check(state, TOK_CARET))
        return make_binary_node(OP_POW, left, parse_power(state));
    
    return left;
}

ast_node * parse_group(parsing_state * state)
{
    if (!consume_check(state, TOK_LBRACKET) && !consume_check(state, TOK_LPAREN))
        return parse_atom(state);
    
    token_type left_paren = last_token(state)->type;

    ast_node* expr = parse_expression(state);

    token_type right_paren = left_paren == TOK_LBRACKET ? TOK_RBRACKET : TOK_RPAREN;

    LOG_ASSERT_ERROR(consume_check(state, right_paren),
        { delete_subtree(expr); return NULL;},
        "Expected '}'.", NULL);
    
    return expr;
}

ast_node * parse_atom(parsing_state * state)
{
    if (consume_check(state, TOK_NUM))
        return make_number_node(last_token(state)->value.num);
    
    if (consume_check(state, TOK_VAR))
    {
        var_name name = last_token(state)->value.name;
        size_t var_id = 0;
        if (!array_try_find_variable(state->variables, name, &var_id))
        {
            array_push(state->variables, name);
            var_id = state->variables->size - 1;
        }
        return make_var_node(*array_get_element(state->variables, var_id));
    }

    LOG_ASSERT_ERROR(0, return NULL,
        "Unexpected token %d", current_token(state)->type);
}

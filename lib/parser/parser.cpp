#include <string.h>

#include "logger.h"

#include "parser.h"

struct parsing_state
{
    const compact_list* tokens;
    list_iterator pos;
    size_t var_cnt;
    char** vars;
};

static ast_node* parse_sum(parsing_state* state);
static ast_node* parse_product(parsing_state* state);
static ast_node* parse_unary(parsing_state* state);
static ast_node* parse_fraction(parsing_state* state);
static ast_node* parse_power(parsing_state* state);
static ast_node* parse_group(parsing_state* state);
static ast_node* parse_atom(parsing_state* state);

static size_t get_var_id(parsing_state* state, const char* name);

static inline token* current_token(parsing_state* state)
{
    return get_element(state->tokens, state->pos);
}

static inline token* last_token(parsing_state* state)
{
    return get_element(state->tokens, prev_element(state->tokens, state->pos));
}

static inline void advance(parsing_state* state)
{
    state->pos = next_element(state->tokens, state->pos);
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

syntax_tree* build_tree(const compact_list * tokens)
{
    syntax_tree* ast = tree_ctor();
    parsing_state state = {
        .tokens  = tokens,
        .pos     = list_begin(tokens),
        .var_cnt = 0,
        .vars    = ast->vars
    };
    ast->root = parse_sum(&state);
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
    ast->var_cnt = state.var_cnt;
    return ast;
}

static ast_node* parse_sum(parsing_state* state)
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
    
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch-enum"
    switch (current_token(state)->type)
    {
    case TOK_MINUS:
        advance(state);
        return make_unary_node(OP_NEG, parse_unary(state));

    #include "functions.h"
    
    default:
        return parse_fraction(state);
    }
    #pragma GCC diagnostic pop

    LOG_ASSERT(0 && "Unreachable code", return NULL);
}

ast_node * parse_fraction(parsing_state * state)
{
    if (!consume_check(state, TOK_FRAC))
        return parse_power(state);

    LOG_ASSERT_ERROR(consume_check(state, TOK_LBRACKET), return NULL,
        "Expected '{' after '\\frac'", NULL);
    
    ast_node* left = parse_sum(state);

    LOG_ASSERT_ERROR(consume_check(state, TOK_RBRACKET),
        {delete_subtree(left); return NULL;},
        "Expected '}' after first argument of '\\frac'", NULL);

    LOG_ASSERT_ERROR(consume_check(state, TOK_LBRACKET), 
        {delete_subtree(left); return NULL;},
        "Expected '{' before second argument of '\\frac'", NULL);

    ast_node* right = parse_sum(state);

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
    
    token_type l_paren = last_token(state)->type;

    ast_node* expr = parse_sum(state);

    if (l_paren == TOK_LBRACKET)
        LOG_ASSERT_ERROR(consume_check(state, TOK_RBRACKET),
            { delete_subtree(expr); return NULL;},
            "Expected '}'.", NULL);
    if (l_paren == TOK_LPAREN)
        LOG_ASSERT_ERROR(consume_check(state, TOK_RPAREN),
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
        char *name = last_token(state)->value.name;
        size_t var_id = get_var_id(state, name);
        return make_var_node(var_id);
    }

    LOG_ASSERT_ERROR(0, return NULL,
        "Unexpected token %d", current_token(state)->type);
}

size_t get_var_id(parsing_state * state, const char * name)
{
    for (size_t i = 0; i < state->var_cnt; i++)
        if (strcmp(name, state->vars[i]) == 0)
            return i;
    LOG_ASSERT_ERROR(state->var_cnt < MAX_VARS, return (size_t)-1,
                        "Too many variables in an expression", NULL);
    state->vars[state->var_cnt] = strdup(name);
    return state->var_cnt++;
}

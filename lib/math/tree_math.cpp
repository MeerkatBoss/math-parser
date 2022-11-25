#include <string.h>

#include "logger.h"

#include "tree_math.h"

static ast_node* copy_tree(ast_node* node);
static ast_node* differential(ast_node* node, size_t var_id);
static int is_const(ast_node* node, size_t var_id);

syntax_tree* derivative(syntax_tree * ast, const char * var)
{
    size_t var_id = ast->var_cnt;
    for (size_t i = 0; i < ast->var_cnt; i++)
        if (strcmp(var, ast->vars[i]) == 0)
            var_id = i;
    LOG_ASSERT_ERROR(var_id < ast->var_cnt, return NULL,
        "Variable '%s' was not defined", var);

    syntax_tree* result = tree_ctor();
    result->var_cnt = ast->var_cnt;
    for (size_t i = 0; i < ast->var_cnt; i++)
        result->vars[i] = strdup(ast->vars[i]);
    
    result->root = differential(ast->root, var_id);

    return result;
}

#define LEFT node->left
#define RIGHT node->right
#define NUM(n) make_number_node(n)
#define ADD(l, r) make_binary_node(OP_ADD, l, r)
#define SUB(l, r) make_binary_node(OP_SUB, l, r)
#define MUL(l, r) make_binary_node(OP_MUL, l, r)
#define FRAC(l, r)make_binary_node(OP_DIV, l, r)
#define POW(l, r) make_binary_node(OP_POW, l, r)
#define NEG(x)    make_unary_node(OP_NEG, x)
#define COS(x)    make_unary_node(OP_COS, x)
#define SIN(x)    make_unary_node(OP_SIN, x)
#define SQRT(x)   make_unary_node(OP_SQRT,x)
#define LN(x)     make_unary_node(OP_LN, x)
#define D(x)      differential(x, var_id)
#define CPY(x)    copy_tree(x)

static ast_node * copy_tree(ast_node * node)
{
    if (!node) return NULL;

    ast_node* res = make_node(node->type, node->value, node->parent);
    res->left   = copy_tree(node->left);
    res->right  = copy_tree(node->right);

    return res;
}

static ast_node * differential(ast_node * node, size_t var_id)
{
    if (is_const(node, var_id))
        return NUM(0);

    if (node->type == T_VAR)
        return NUM(1);
    
    #define MATH_FUNC(name, diff, ...)\
        case OP_##name:\
            return MUL(diff, D(RIGHT));

    switch(node->value.op)
    {
        case OP_ADD:
            return ADD(D(LEFT), D(RIGHT));
        case OP_SUB:
            return SUB(D(LEFT), D(RIGHT));
        case OP_MUL:
            return ADD(MUL(D(LEFT), CPY(RIGHT)), MUL(CPY(LEFT), D(RIGHT)));
        case OP_DIV:
            return FRAC(
                SUB(
                    MUL(D(LEFT), CPY(RIGHT)),
                    MUL(CPY(LEFT), D(RIGHT))
                ),
                POW(CPY(RIGHT), NUM(2))
            );
        case OP_POW:
            if(is_const(node->left, var_id))
                return MUL(
                    MUL(
                        LN(CPY(LEFT)),
                        CPY(node)
                    ),
                    D(RIGHT)
                );
            if (is_const(node->right, var_id))
                return MUL(
                    MUL(
                        CPY(RIGHT),
                        POW(
                            CPY(LEFT),
                            SUB(CPY(RIGHT), NUM(1))
                        )
                    ),
                    D(RIGHT)
                );
            return MUL(
                CPY(node),
                ADD(
                    MUL(
                        D(RIGHT),
                        LN(CPY(LEFT))
                    ),
                    MUL(
                        CPY(RIGHT),
                        FRAC(
                            D(LEFT),
                            CPY(LEFT)
                        )
                    )
                )
            );
        case OP_NEG:
            return NEG(D(RIGHT));
        #include "functions.h"

        default:
            LOG_ASSERT_ERROR(0, return NULL,
                "Unknown node", NULL);
    }

    #undef MATH_FUNC

    LOG_ASSERT(0 && "Unreachable code", return NULL);
}

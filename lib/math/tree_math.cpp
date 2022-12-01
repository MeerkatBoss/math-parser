#include <string.h>
#include <math.h>

#include "logger.h"

#include "math_utils.h"
#include "tree_math.h"

static ast_node* get_differential(ast_node* node, size_t var_id);
static void simplify_node(ast_node* node);
static int is_const(ast_node* node, size_t var_id);
static ast_node* evaluate_partially(ast_node* node, size_t var_id, double val);

abstract_syntax_tree* derivative(abstract_syntax_tree * ast, const char * var)
{
    size_t var_id = ast->var_cnt;
    for (size_t i = 0; i < ast->var_cnt; i++)
        if (strcmp(var, ast->vars[i]) == 0) // TODO: cheto takoe videl uze, extract!!
            var_id = i;
    LOG_ASSERT_ERROR(var_id < ast->var_cnt, return NULL,
        "Variable '%s' was not defined", var);

    abstract_syntax_tree* result = tree_ctor();
    result->var_cnt = ast->var_cnt;
    for (size_t i = 0; i < ast->var_cnt; i++)
        result->vars[i] = strdup(ast->vars[i]); // TODO: many bukva, malo smisla, extract!
    
    result->root = get_differential(ast->root, var_id);
    simplify(result);

    return result;
}

void simplify(abstract_syntax_tree * ast)
{
    simplify_node(ast->root);
}

#define LEFT node->left
#define RIGHT node->right

static inline ast_node* NUM(double num)    { return make_number_node(num); }
static inline ast_node* VAR(size_t var_id) { return make_var_node(var_id); }

static inline ast_node* ADD (ast_node* left, ast_node* right) { return make_binary_node(OP_ADD, left, right); }
static inline ast_node* SUB (ast_node* left, ast_node* right) { return make_binary_node(OP_SUB, left, right); }
static inline ast_node* MUL (ast_node* left, ast_node* right) { return make_binary_node(OP_MUL, left, right); }
static inline ast_node* FRAC(ast_node* left, ast_node* right) { return make_binary_node(OP_DIV, left, right); }
static inline ast_node* POW (ast_node* left, ast_node* right) { return make_binary_node(OP_POW, left, right); }

static inline ast_node* NEG (ast_node* right) { return make_unary_node(OP_NEG, right); }
static inline ast_node* SIN (ast_node* right) { return make_unary_node(OP_SIN, right); }
static inline ast_node* COS (ast_node* right) { return make_unary_node(OP_COS, right); }
static inline ast_node* SQRT(ast_node* right) { return make_unary_node(OP_SQRT, right); }
static inline ast_node* LN  (ast_node* right) { return make_unary_node(OP_LN, right); }

#define D(x) get_differential(x, var_id)

static inline ast_node* CPY(ast_node* node) { return copy_tree(node); }

abstract_syntax_tree* maclaurin_series(abstract_syntax_tree * ast, const char * var, int pow)
{
    size_t var_id = ast->var_cnt;
    for (size_t i = 0; i < ast->var_cnt; i++)
        if (strcmp(var, ast->vars[i]) == 0) // TODO: mnoga bukov malo smisla
            var_id = i;
    LOG_ASSERT_ERROR(var_id < ast->var_cnt, return NULL,
        "Variable '%s' was not defined", var);

    abstract_syntax_tree* result = tree_ctor();
    result->var_cnt = ast->var_cnt;
    for (size_t i = 0; i < ast->var_cnt; i++)
        result->vars[i] = strdup(ast->vars[i]); // TODO: mnoga bukov malo smisla
    result->root = NUM(0);
    
    ast_node* cur = CPY(ast->root);
    for (int i = 0; i <= pow; i++)
    {
        ast_node* at_zero = evaluate_partially(cur, var_id, 0);
        simplify_node(at_zero);
        result->root =
            ADD(
                result->root,
                MUL(
                    at_zero,
                    FRAC(
                        POW(VAR(var_id), NUM(i)),
                        NUM(factorial(i))
                    )
                )
            );
        ast_node* nxt = get_differential(cur, var_id);
        delete_subtree(cur);
        cur = nxt;
    }
    simplify_node(result->root);
    delete_subtree(cur);

    return result;
}

static ast_node* get_differential(ast_node * node, size_t var_id)
{
    if (is_const(node, var_id))
        return NUM(0);

    if (node->type == NODE_NUM)
        return NUM(1);
    
    #define MATH_FUNC(name, diff, ...)\
        case OP_##name:\
            return MUL(diff, D(RIGHT));

    switch(node->value.op)
    { // TODO: codgen?
        case OP_ADD:
            return ADD(D(LEFT), D(RIGHT));
        case OP_SUB:
            return SUB(D(LEFT), D(RIGHT));
        case OP_MUL:
            return ADD(
                MUL(D(LEFT), CPY(RIGHT)),
                MUL(CPY(LEFT), D(RIGHT))
            );
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
                    D(LEFT)
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

static void replace_with(ast_node* dest, ast_node* src);

static inline int is_neg    (ast_node* node) { return op_cmp(node, OP_NEG); }
static inline int is_neutral(ast_node* node) { return num_cmp(node, 0) || num_cmp(node, 1); }

static void extract_negative(ast_node* node);
static void extract_left_negative(ast_node* node);
static void extract_right_negative(ast_node* node);
static void extract_left_neutral(ast_node* node);
static void extract_right_neutral(ast_node* node);
static void collapse_const(ast_node* node);

static void simplify_node(ast_node * node)
{
    if (is_num(node)) extract_negative(node);
    if (!is_op(node)) return;

    simplify_node(node->left);
    simplify_node(node->right);

    if (is_num(LEFT) && is_num(RIGHT)) collapse_const        (node);
    if (is_neg(LEFT) && is_neg(RIGHT)) extract_negative      (node);
    if (is_neg(LEFT))                  extract_left_negative (node);
    if (is_neg(RIGHT))                 extract_right_negative(node);
    if (is_neutral(LEFT))              extract_left_neutral  (node);
    if (is_neutral(RIGHT))             extract_right_neutral (node);
}

static int is_const(ast_node * node, size_t var_id)
{
    if (!node) return 1; /* Vacuous truth */

    if (is_num(node)) return 1;
    if (is_var(node)) return !var_cmp(node, var_id);

    return is_const(LEFT, var_id) && is_const(RIGHT, var_id);
}

ast_node* evaluate_partially(ast_node* node, size_t var_id, double val)
{
    if (!node) return NULL;
    if (node->type == NODE_NUM && node->value.var_id == var_id)
        return NUM(val);
    
    ast_node* copy = make_node(node->type, node->value);
    copy-> left = evaluate_partially(node-> left, var_id, val);
    copy->right = evaluate_partially(node->right, var_id, val);
    if (copy-> left) copy-> left->parent = copy;
    if (copy->right) copy->right->parent = copy;

    return copy;
}
static void replace_with(ast_node* dest, ast_node* src)
{
    dest->left  = src->left;
    dest->right = src->right;
    if (dest->left)  dest->left ->parent = dest;
    if (dest->right) dest->right->parent = dest;
    dest->type = src->type;
    dest->value = src->value;
    src->left  = NULL;
    src->right = NULL;

    delete_node(src);
}

static inline void assign_num(ast_node* dest, double num)
{
    dest->value.num = num;
    dest->type = NODE_NUM;
    if (dest-> left) delete_subtree(dest-> left);
    if (dest->right) delete_subtree(dest->right);
    dest->left = NULL;
    dest->right = NULL;
}

#define ASSIGN_NODE(num) assign_num(node, num)

static void collapse_const(ast_node* node)
{
    #define COMBINE_CHILDREN(op) ASSIGN_NODE(get_num(LEFT) op get_num(RIGHT))
    LOG_ASSERT(is_op(node), return);
    LOG_ASSERT(is_num(LEFT), return);
    LOG_ASSERT(is_num(RIGHT), return);

    switch (get_op(node))
    {
    case OP_ADD: COMBINE_CHILDREN(+); break;
    case OP_SUB: COMBINE_CHILDREN(-); break;
    case OP_MUL: COMBINE_CHILDREN(*); break;
    case OP_DIV: COMBINE_CHILDREN(/); break;
    case OP_POW: ASSIGN_NODE(pow(get_num(LEFT), get_num(RIGHT))); break;
    default:
        break;
    }

    #undef COMBINE_CHILDREN
}

void extract_negative(ast_node * node)
{
    LOG_ASSERT(node, return);
    if (is_num(node) && get_num(node) < 0)
    {
        node->right = make_number_node(-1 * get_num(node));
        node->type = NODE_OP;
        node->value.op = OP_NEG;
        return;
    }

    LOG_ASSERT(is_op(node), return);
    LOG_ASSERT(op_cmp(LEFT,  OP_NEG), return);
    LOG_ASSERT(op_cmp(RIGHT, OP_NEG), return);

    switch(get_op(node))
    {
    case OP_ADD:
    case OP_SUB:
        replace_with(node, NEG(
            make_binary_node(
                get_op(node),
                LEFT->right,
                RIGHT->right)));
        simplify_node(RIGHT);
        break;
    case OP_MUL:
    case OP_DIV:
        replace_with(LEFT,  LEFT ->right);
        replace_with(RIGHT, RIGHT->right);
        break;
    default:
        break;
    }
}

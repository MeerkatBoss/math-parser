#include <string.h>
#include <math.h>

#include "logger.h"

#include "tree_math.h"

static const double EPS = 1e-6;

static ast_node* copy_tree(ast_node* node);
static ast_node* differential(ast_node* node, size_t var_id);
static void simplify_node(ast_node* node);
static int is_const(ast_node* node, size_t var_id);
static ast_node* value_at_position(ast_node* node, size_t var_id, double val);
static inline long long factorial(long long n)
{
    long long res = 1;
    for (long long i = 1; i <= n; i++)
        res *= i;
    return res;
}

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
    simplify(result);

    return result;
}

void simplify(syntax_tree * ast)
{
    simplify_node(ast->root);
}

#define LEFT node->left
#define RIGHT node->right
#define NUM(n) make_number_node(n)
#define VAR(x) make_var_node(x)
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

syntax_tree * maclaurin_series(syntax_tree * ast, const char * var, int pow)
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
    result->root = NUM(0);
    
    ast_node* cur = CPY(ast->root);
    for (int i = 0; i <= pow; i++)
    {
        ast_node* at_zero = value_at_position(cur, var_id, 0);
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
        ast_node* nxt = differential(cur, var_id);
        delete_subtree(cur);
        cur = nxt;
    }
    simplify_node(result->root);
    delete_subtree(cur);

    return result;
}

static ast_node * copy_tree(ast_node * node)
{
    if (!node) return NULL;

    ast_node* res = make_node(node->type, node->value);
    res->left   = copy_tree(node->left);
    res->right  = copy_tree(node->right);
    if (res-> left) res-> left->parent = res;
    if (res->right) res->right->parent = res;

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

static void simplify_sum(ast_node* node);
static void simplify_product(ast_node* node);
static void simplify_negative(ast_node* node);
static void simplify_func(ast_node* node);
static void simplify_fraction(ast_node* node);
static void simplify_power(ast_node* node);
static void replace_with(ast_node* dest, ast_node* src);

static void simplify_node(ast_node * node)
{
    if (!node) return;
    if (node->type == T_NUM && node->value.num < 0)
    {
        node->right = NUM(-1 * node->value.num);
        node->type = T_OP;
        node->value.op = OP_NEG;
        return;
    }
    if (node->type != T_OP) return;

    simplify_node(node->left);
    simplify_node(node->right);

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch-enum"
    switch (node->value.op)
    {
    case OP_ADD:
    case OP_SUB:
        return simplify_sum(node);
    case OP_MUL:
        return simplify_product(node);
    case OP_DIV:
        return simplify_fraction(node);
    case OP_NEG:
        return simplify_negative(node);
    case OP_POW:
        return simplify_power(node);
    
    default:
        return simplify_func(node);
    }
    #pragma GCC diagnostic pop
}

static int is_const(ast_node * node, size_t var_id)
{
    if (!node) return 1;
    if (node->type == T_NUM)
        return 1;
    if (node->type == T_VAR)
        return node->value.var_id == var_id ? 0 : 1;
    return is_const(node->left, var_id) && is_const(node->right, var_id);
}

ast_node * value_at_position(ast_node * node, size_t var_id, double val)
{
    if (!node) return NULL;
    if (node->type == T_VAR && node->value.var_id == var_id)
        return NUM(val);
    
    ast_node* copy = make_node(node->type, node->value);
    copy-> left = value_at_position(node-> left, var_id, val);
    copy->right = value_at_position(node->right, var_id, val);
    if (copy-> left) copy-> left->parent = copy;
    if (copy->right) copy->right->parent = copy;

    return copy;
}

static void simplify_sum(ast_node * node)
{
    if (node->left ->type == T_NUM &&
        node->right->type == T_NUM)
    {
        node->type = T_NUM;
        double l = node->left ->value.num;
        double r = node->right->value.num;
        if (node->value.op == OP_ADD)
            node->value.num = l + r;
        else
            node->value.num = l - r;

        delete_node(node->left);
        delete_node(node->right);

        node->left = NULL;
        node->right = NULL;
        return;
    }
    if (node->left ->type == T_VAR &&
        node->right->type == T_VAR &&
        node->left->value.var_id == node->right->value.var_id)
    {
        size_t var = node->left->value.var_id;
        delete_node(node->left);
        delete_node(node->right);
        node->left = NULL;
        node->right = NULL;
        if (node->value.op == OP_ADD)
        {
            node->value.op = OP_MUL;
            node->left  = NUM(2);
            node->right = VAR(var);
            return;
        }
        node->type = T_NUM;
        node->value.num = 0;
    }

    if (node->left->type == T_NUM && fabs(node->left->value.num) < EPS)
    {
        delete_node(node->left);
        node->left = NULL;

        if (node->value.op == OP_ADD)
            replace_with(node, node->right);
        else
            node->value.op = OP_NEG;
        return;
    }
    if (node->right->type == T_NUM && fabs(node->right->value.num) < EPS)
    {
        delete_node(node->right);

        replace_with(node, node->left);
        return;
    }

    if (node->left ->type == T_OP && node->left ->value.op == OP_NEG &&
        node->right->type == T_OP && node->right->value.op == OP_NEG)
    {
        replace_with(node-> left, node->left ->right);
        replace_with(node->right, node->right->right);
        if (node->value.op == OP_SUB)
        {
            ast_node* tmp = node->left;
            node->left = node->right;
            node->right = tmp;
            
            return;
        }

        replace_with(node, NEG(ADD(LEFT, RIGHT)));
        simplify_sum(node->right);
        return;
    }
    if (node->left ->type == T_OP && node->left ->value.op == OP_NEG)
    {
        replace_with(node-> left, node->left ->right);
        if (node->value.op == OP_SUB)
        {
            replace_with(node, NEG(ADD(LEFT, RIGHT)));
            simplify_sum(node->right);
            return;
        }

        replace_with(node, SUB(RIGHT, LEFT));
        simplify_sum(node);
        return;
    }
    if (node->right->type == T_OP && node->right->value.op == OP_NEG)
    {
        replace_with(node->right, node->right->right);
        if (node->value.op == OP_SUB)
        {
            replace_with(node, ADD(LEFT, RIGHT));
            simplify_sum(node);
            return;
        }

        replace_with(node, SUB(LEFT, RIGHT));
        simplify_sum(node);
        return;
    }
}

void simplify_product(ast_node * node)
{
    if (node->left ->type == T_NUM &&
        node->right->type == T_NUM)
    {
        node->type = T_NUM;
        double l = node->left ->value.num;
        double r = node->right->value.num;
        node->value.num = l * r;

        delete_node(node->left);
        delete_node(node->right);

        node->left = NULL;
        node->right = NULL;
        return;
    }
    if (node->left ->type == T_VAR &&
        node->right->type == T_VAR &&
        node->left->value.var_id == node->right->value.var_id)
    {
        size_t var = node->left->value.var_id;
        delete_node(node->left);
        delete_node(node->right);
        node->left = NULL;
        node->right = NULL;
        node->value.op = OP_POW;

        node->left  = VAR(var);
        node->right = NUM(2);
        return;
    }
    if (node->left ->type == T_NUM && fabs(node->left ->value.num) < EPS ||
        node->right->type == T_NUM && fabs(node->right->value.num) < EPS)
    {
        delete_subtree(node->left);
        delete_subtree(node->right);
        node->left = NULL;
        node->right = NULL;

        node->type = T_NUM;
        node->value.num = 0;

        return;
    }
    if (node->left ->type == T_OP && node->left ->value.op == OP_NEG &&
        node->right->type == T_OP && node->right->value.op == OP_NEG)
    {
        replace_with(node->left,  node->left ->right);
        replace_with(node->right, node->right->right);
        return;
    }
    if (node->left ->type == T_OP && node->left ->value.op == OP_NEG)
    {
        replace_with(node->left,  node->left ->right);
        replace_with(node, NEG(MUL(LEFT, RIGHT)));
        simplify_product(node->right);
        simplify_negative(node);

        return;
    }
    if (node->right->type == T_OP && node->right->value.op == OP_NEG)
    {
        replace_with(node->right, node->right->right);
        replace_with(node, NEG(MUL(LEFT, RIGHT)));
        simplify_product(node->right);
        simplify_negative(node);

        return;
    }
    if (node->left->type == T_NUM && fabs(node->left->value.num - 1) < EPS)
    {
        delete_node(node->left);
        node->left = NULL;

        replace_with(node, node->right);

        return;
    }
    if (node->right->type == T_NUM && fabs(node->right->value.num - 1) < EPS)
    {
        delete_node(node->right);
        node->right = NULL;

        replace_with(node, node->left);

        return;
    }
}

void simplify_negative(ast_node * node)
{
    if (node->right->type == T_OP && node->right->value.op == OP_NEG)
    {
        ast_node* tmp = node->right;
        replace_with(node, tmp->right);

        tmp->right = NULL;
        delete_node(tmp);

        return;
    }

    if (node->right->type == T_OP && node->right->value.op == OP_SUB)
    {
        ast_node* tmp = node->right->left;
        node->right->left = node->right->right;
        node->right->right = tmp;
        return;
    }

    if (node->right->type == T_NUM && fabs(node->right->value.num) < EPS)
    {
        delete_node(node->right);

        replace_with(node, NUM(0));
        return;
    }
}

void simplify_func(ast_node * node)
{
    if (node->value.op == OP_LN && node->right->type == T_NUM &&
        fabs(node->right->value.num - 1) < EPS)
    {
        node->type = T_NUM;
        node->value.num = 0;

        delete_node(node->right);

        node->right = NULL;
        return;

    }
    if (node->value.op == OP_SQRT && node->right->type == T_NUM &&
        fabs(node->right->value.num - 1) < EPS)
    {
        node->type = T_NUM;
        node->value.num = 1;

        delete_node(node->right);

        node->right = NULL;
        return;

    }
    if (node->value.op == OP_SQRT && node->right->type == T_NUM &&
        fabs(node->right->value.num) < EPS)
    {
        node->type = T_NUM;
        node->value.num = 0;

        delete_node(node->right);

        node->right = NULL;
        return;

    }
    if (node->value.op == OP_SIN || node->value.op == OP_ARCSIN ||
        node->value.op == OP_TAN || node->value.op == OP_ARCTAN  ||
        node->value.op == OP_COT || node->value.op == OP_ARCCOT)
    {
        if(node->right->type == T_OP && node->right->value.op == OP_NEG)
        {
            node->right->value.op = node->value.op;
            node->value.op = OP_NEG;
            return;
        }
        if (node->right->type == T_NUM && fabs(node->right->value.num) < EPS)
        {
            delete_node(node->right);

            replace_with(node, NUM(0));
            return;
        }
    }
    
    if (node->value.op == OP_COS)
    {
        if(node->right->type == T_OP && node->right->value.op == OP_NEG)
        {
            replace_with(node->right, node->right->right);
            return;
        }
        if (node->right->type == T_NUM && fabs(node->right->value.num) < EPS)
        {
            delete_node(node->right);

            replace_with(node, NUM(1));
            return;
        }
    }
}

void simplify_fraction(ast_node * node)
{
    if (node->left ->type == T_NUM &&
        node->right->type == T_NUM)
    {
        node->type = T_NUM;
        double l = node->left ->value.num;
        double r = node->right->value.num;
        node->value.num = l / r;

        delete_node(node->left);
        delete_node(node->right);

        node->left = NULL;
        node->right = NULL;
        return;
    }

    if (node->left ->type == T_NUM && fabs(node->left ->value.num) < EPS)
    {
        delete_node(node->left);
        delete_subtree(node->right);
        node->left = NULL;
        node->right = NULL;

        node->type = T_NUM;
        node->value.num = 0;

        return;
    }
    if (node->right->type == T_NUM && fabs(node->right->value.num - 1) < EPS)
    {
        delete_node(node->right);
        node->right = NULL;

        replace_with(node, node->left);

        return;
    }
    if (node->right->type == T_NUM && fabs(node->right->value.num + 1) < EPS)
    {
        delete_node(node->right);
        node->right = node->left;
        node->left = NULL;

        node->value.op = OP_NEG;

        return;
    }
    if (node->left ->type == T_OP && node->left ->value.op == OP_NEG)
    {
        replace_with(node->left,  node->left ->right);
        replace_with(node, NEG(FRAC(LEFT, RIGHT)));
        simplify_fraction(node->right);
        simplify_negative(node);

        return;
    }
}

void simplify_power(ast_node * node)
{
    if (node->left ->type == T_NUM &&
        node->right->type == T_NUM)
    {
        node->type = T_NUM;
        double l = node->left ->value.num;
        double r = node->right->value.num;
        node->value.num = pow(l, r);

        delete_node(node->left);
        delete_node(node->right);

        node->left = NULL;
        node->right = NULL;
        return;
    }
    if (node->left ->type == T_NUM && fabs(node->left ->value.num) < EPS)
    {
        delete_node(node->left);
        delete_subtree(node->right);
        node->left = NULL;
        node->right = NULL;

        node->type = T_NUM;
        node->value.num = 0;

        return;
    }
    if (node->right->type == T_NUM && fabs(node->right->value.num) < EPS)
    {
        delete_subtree(node->left);
        delete_node(node->right);

        replace_with(node, NUM(1));
        return;
    }
    if (node->right->type == T_NUM && fabs(node->right->value.num - 1) < EPS)
    {
        delete_node(node->right);

        replace_with(node, node->left);
        return;
    }
    if (node->right->type == T_NUM && node->left->type == T_OP &&
        node->left->value.op == OP_NEG)
    {
        int pw = round(node->right->value.num);
        if (fabs(node->right->value.num - pw) >= EPS)
            return;
        if (pw % 2 == 0)
        {
            replace_with(node->left, node->left->right);
            return;
        }
        node->left->left = node->left->right;
        node->left->left->parent = node->left;

        node->left->right = node->right;
        node->left->right->parent = node->left;

        node->left->value.op = OP_POW;
        node->right = node->left;
        node->left = NULL;
        node->value.op = OP_NEG;
        return;
    }
}

static void replace_with(ast_node * dest, ast_node * src)
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

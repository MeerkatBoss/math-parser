#include <string.h>
#include <math.h>

#include "logger.h"

#include "math_utils.h"
#include "tree_math.h"

static ast_node* get_differential(ast_node* node, var_name var, article_builder* article, int no_print = 1);
static void simplify_node(ast_node* node);
static int is_const(ast_node* node, var_name var);
static ast_node* evaluate_partially(ast_node* node, var_name var, double val);

abstract_syntax_tree* derivative(abstract_syntax_tree * ast, const char * var, article_builder* article)
{
    size_t var_id = 0;
    LOG_ASSERT_ERROR(
        array_try_find_variable(&ast->variables, var, &var_id),
        return NULL,
        "Variable '%s' was not defined", var);
    
    var_name v_name = *array_get_element(&ast->variables, var_id);

    string_builder_append(&article->text, "Let us find the derivative of the following function:\n");
    print_node(ast->root, &article->text);
    
    abstract_syntax_tree* result = tree_copy(ast);
    
    result->root = get_differential(ast->root, v_name, article);

    string_builder_append(&article->text, "\nNow the proof that the derivative of this function is equal to\n");
    print_node(result->root, &article->text);
    article_add_placeholder(article);

    article_add_transition(article);
    string_builder_append(&article->text, "if we simplify this we wil get\n");
    simplify(result);
    print_node(result->root, &article->text);


    return result;
}

void simplify(abstract_syntax_tree* ast)
{
    simplify_node(ast->root);
}

#define LEFT node->left
#define RIGHT node->right

static inline ast_node* NUM(double num)    { return make_number_node(num); }
static inline ast_node* VAR(var_name var)  { return make_var_node(var);    }

static inline ast_node* ADD (ast_node* left, ast_node* right) { return make_binary_node(OP_ADD, left, right); }
static inline ast_node* SUB (ast_node* left, ast_node* right) { return make_binary_node(OP_SUB, left, right); }
static inline ast_node* MUL (ast_node* left, ast_node* right) { return make_binary_node(OP_MUL, left, right); }
static inline ast_node* FRAC(ast_node* left, ast_node* right) { return make_binary_node(OP_DIV, left, right); }
static inline ast_node* POW (ast_node* left, ast_node* right) { return make_binary_node(OP_POW, left, right); }

static inline ast_node* NEG (ast_node* right) { return make_unary_node(OP_NEG, right); }
static inline ast_node* SIN (ast_node* right) { return make_unary_node(OP_SIN, right); }
static inline ast_node* COS (ast_node* right) { return make_unary_node(OP_COS, right); }
static inline ast_node* SQRT(ast_node* right) { return make_unary_node(OP_SQRT, right);}
static inline ast_node* LN  (ast_node* right) { return make_unary_node(OP_LN, right);  }

#define D(x) get_differential(x, var, article, print)

static inline ast_node* CPY(ast_node* node) { return copy_subtree(node); }

abstract_syntax_tree* maclaurin_series(abstract_syntax_tree * ast, const char * var, int pow, article_builder* article)
{
    size_t var_id = 0;
    LOG_ASSERT_ERROR(
        array_try_find_variable(&ast->variables, var, &var_id),
        return NULL,
        "Variable '%s' was not defined", var);

    string_builder_append_format(&article->text, "Let us find the Taylor series"
                                                 " at $%s = %g$ of the following function:\n",
                                                 var, 0.0);
    print_node(ast->root, &article->text);

    abstract_syntax_tree* result = tree_copy(ast);
    result->root = make_number_node(0);

    var_name v_name = *array_get_element(&ast->variables, var_id);
    
    ast_node* cur = CPY(ast->root);
    for (int i = 0; i <= pow; i++)
    {
        ast_node* at_zero = evaluate_partially(cur, v_name, 0);
        simplify_node(at_zero);
        result->root =
            ADD(
                result->root,
                MUL(
                    at_zero,
                    FRAC(
                        POW(VAR(v_name), NUM(i)),
                        NUM(factorial(i))
                    )
                )
            );
        ast_node* nxt = get_differential(cur, v_name, article);
        delete_subtree(cur);
        cur = nxt;
        simplify_node(cur);
    }
    delete_subtree(cur);

    string_builder_append_format(&article->text, "\nNow the proof that the Taylor series of this function"
                                                 " at $%s = %g$ is equal to\n", var, 0.0);
    print_node(result->root, &article->text);
    article_add_placeholder(article);

    article_add_transition(article);
    string_builder_append(&article->text, "if we simplify this we wil get\n");
    simplify(result);
    print_node(result->root, &article->text);

    return result;
}

static ast_node* get_differential(ast_node * node, var_name var, article_builder* article, int print)
{
    if (print && !is_op(LEFT) && !is_op(RIGHT))
    {
        article_add_starter(article);
        print_node(node, &article->text);
        article_add_transition(article);

        ast_node* result = get_differential(node, var, article, 0);
        string_builder_append(&article->text, "the derivative of this is equal to\n");
        print_node(result, &article->text);
        
        return result;
    }

    if (var_cmp(node, var))
        return NUM(1);

    if (is_const(node, var))
        return NUM(0);
    
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
            if(is_const(node->left, var))
                return MUL(
                    MUL(
                        LN(CPY(LEFT)),
                        CPY(node)
                    ),
                    D(RIGHT)
                );
            if (is_const(node->right, var))
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

static inline int is_neg (ast_node* node) { return op_cmp(node, OP_NEG); }
static inline int is_zero(ast_node* node) { return num_cmp(node, 0); }
static inline int is_one (ast_node* node) { return num_cmp(node, 1); }
static inline int get_int(ast_node* node) { return (int) round(get_num(node)); }
static inline int is_int (ast_node* node) { return num_cmp(node, get_int(node)); }

static inline int is_same_var(ast_node* node1, ast_node* node2)
{
    return is_var(node1) && var_cmp(node2, get_var(node1));
}

static void extract_negative      (ast_node* node);
static void extract_left_negative (ast_node* node);
static void extract_right_negative(ast_node* node);
static void extract_left_zero     (ast_node* node);
static void extract_right_zero    (ast_node* node);
static void extract_left_one      (ast_node* node);
static void extract_right_one     (ast_node* node);
static void collapse_var          (ast_node* node);
static void collapse_const        (ast_node* node);

static void simplify_node(ast_node * node)
{
    if (is_num(node)) extract_negative(node);
    if (!is_op(node)) return;

    simplify_node(node->left);
    simplify_node(node->right);

    if (is_neg(LEFT) && is_neg(RIGHT)) extract_negative      (node);
    if (is_neg(LEFT))                  extract_left_negative (node);
    if (is_neg(RIGHT))                 extract_right_negative(node);
    if (is_zero(LEFT))                 extract_left_zero     (node);
    if (is_zero(RIGHT))                extract_right_zero    (node);
    if (is_one(LEFT))                  extract_left_one      (node);
    if (is_one(RIGHT))                 extract_right_one     (node);
    if (is_num(LEFT) && is_num(RIGHT)) collapse_const        (node);
    if (is_same_var(LEFT, RIGHT))      collapse_var          (node);
}

static int is_const(ast_node * node, var_name var)
{
    if (!node) return 1; /* Vacuous truth */

    if (is_num(node)) return 1;
    if (is_var(node)) return !var_cmp(node, var);

    return is_const(LEFT, var) && is_const(RIGHT, var);
}

ast_node* evaluate_partially(ast_node* node, var_name var, double val)
{
    if (!node) return NULL;
    if (var_cmp(node, var))
        return NUM(val);
    
    ast_node* copy = make_node(node->type, node->value);
    copy-> left = evaluate_partially(node-> left, var, val);
    copy->right = evaluate_partially(node->right, var, val);
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

static void collapse_const(ast_node* node)
{
    #define ASSIGN_NODE(num) assign_num(node, num)
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
    #undef ASSIGN_NODE
}

void extract_negative(ast_node* node)
{
    LOG_ASSERT(node, return);
    if (is_num(node))
    {
        if (get_num(node) < 0)
        {
            node->right = make_number_node(-1 * get_num(node));
            node->type = NODE_OP;
            node->value.op = OP_NEG;
        }
        return;
    }

    LOG_ASSERT(is_op(node), return);
    LOG_ASSERT(op_cmp(LEFT,  OP_NEG), return);
    LOG_ASSERT(op_cmp(RIGHT, OP_NEG), return);

    switch(get_op(node))
    {
    case OP_ADD:
    case OP_SUB:
        replace_with(LEFT,  LEFT ->right);
        replace_with(RIGHT, RIGHT->right);
        replace_with(node, NEG(
            make_binary_node(
                get_op(node),
                LEFT,
                RIGHT)));
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

static void extract_left_negative(ast_node* node)
{
    LOG_ASSERT(is_op(node), return);
    LOG_ASSERT(op_cmp(LEFT, OP_NEG), return);

    switch(get_op(node))
    {
    case OP_ADD:
        replace_with(LEFT, LEFT->right);
        replace_with(node, SUB(RIGHT, LEFT));
        break;
    case OP_SUB:
        replace_with(LEFT, LEFT->right);
        replace_with(node, NEG(ADD(LEFT, RIGHT)));
        break;
    case OP_MUL:
    case OP_DIV:
        replace_with(LEFT, LEFT->right);
        replace_with(node, NEG(
            make_binary_node(
                get_op(node),
                LEFT,
                RIGHT)));
        break;
    case OP_POW:
        if (!is_int(RIGHT)) break;
        replace_with(LEFT, LEFT->right);
        if (get_int(RIGHT) % 2 == 1)
            replace_with(node, NEG(POW(LEFT, RIGHT)));
        break;
    default:
        break;
    }
}

static void extract_right_negative(ast_node* node)
{
    LOG_ASSERT(is_op(node), return);
    LOG_ASSERT(op_cmp(RIGHT, OP_NEG), return);

    switch (get_op(node))
    {
    case OP_ADD:
        replace_with(RIGHT, RIGHT->right);
        replace_with(node, SUB(LEFT, RIGHT));
        break;;
    case OP_SUB:
        replace_with(RIGHT, RIGHT->right);
        replace_with(node, ADD(LEFT, RIGHT));
        break;
    case OP_MUL:
    case OP_DIV:
        replace_with(RIGHT, RIGHT->right);
        replace_with(node, NEG(
            make_binary_node(
                get_op(node),
                LEFT,
                RIGHT)));
        break;
    case OP_NEG:
        replace_with(RIGHT, RIGHT->right);
        replace_with(node, RIGHT);
        break;
    case OP_SIN:
    case OP_ARCSIN:
    case OP_TAN:
    case OP_ARCTAN:
        replace_with(RIGHT, RIGHT->right);
        replace_with(node, NEG(
            make_unary_node(
                get_op(node),
                RIGHT)));
        break;
    case OP_COS:
        replace_with(RIGHT, RIGHT->right);
        break;
    default:
        break;
    }

}

static void extract_left_zero(ast_node* node)
{
    LOG_ASSERT(is_op(node), return);
    LOG_ASSERT(is_zero(LEFT), return);

    switch(get_op(node))
    {
    case OP_ADD:
        delete_node(LEFT);
        replace_with(node, RIGHT);
        break;
    case OP_SUB:
        delete_node(LEFT);
        replace_with(node, NEG(RIGHT));
        break;
    case OP_MUL:
    case OP_DIV:
        assign_num  (node, 0);
        break;
    default:
        break;
    }
}

static void extract_right_zero(ast_node* node)
{
    LOG_ASSERT(is_op(node), return);
    LOG_ASSERT(is_zero(RIGHT), return);
    LOG_ASSERT_ERROR(!op_cmp(node, OP_DIV), return, "Division by zero", NULL);
    LOG_ASSERT_ERROR(!op_cmp(node, OP_LN), return, "Logarithm of zero is undefined", NULL);

    switch (get_op(node))
    {
    case OP_ADD:
    case OP_SUB:
        delete_node(RIGHT);
        replace_with(node, LEFT);
        break;
    case OP_MUL:
    case OP_NEG:
    case OP_SIN:
    case OP_TAN:
    case OP_SQRT:
    case OP_ARCSIN:
    case OP_ARCTAN:
        assign_num(node, 0);
        break;
    case OP_POW:
    case OP_COS:
        assign_num(node, 1);
        break;
    default:
        break;
    }
}

static void extract_left_one(ast_node *node)
{
    LOG_ASSERT(is_op(node), return);
    LOG_ASSERT(is_one(LEFT), return);

    switch (get_op(node))
    {
    case OP_MUL:
        delete_node(LEFT);
        replace_with(node, RIGHT);
        break;
    case OP_POW:
        assign_num  (node, 1);
        break;
    default:
        break;
    }
}

static void extract_right_one(ast_node *node)
{
    LOG_ASSERT(is_op(node), return);
    LOG_ASSERT(is_one(RIGHT), return);

    switch (get_op(node))
    {
    case OP_MUL:
    case OP_DIV:
    case OP_POW:
        delete_node(RIGHT);
        replace_with(node, LEFT);
        break;
    case OP_LN:
        assign_num(node, 0);
        break;
    case OP_SQRT:
        assign_num(node, 1);
        break;
    default:
        break;
    }
}

static void collapse_var(ast_node *node)
{
    LOG_ASSERT(is_op(node), return);
    LOG_ASSERT(is_same_var(LEFT, RIGHT), return);

    switch (get_op(node))
    {
    case OP_ADD:
        delete_node(LEFT);
        replace_with(node, MUL(NUM(2), RIGHT));
        break;
    case OP_SUB:
        assign_num(node, 0);
        break;
    case OP_MUL:
        delete_node(RIGHT);
        replace_with(node, POW(LEFT, NUM(2)));  break;
        break;
    case OP_DIV:
        assign_num(node, 1);
        break;
    default:
        break;
    }
}

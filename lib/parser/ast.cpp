#include <stdlib.h>
#include <string.h>

#include "logger.h"

#include "ast.h"

ast_node* make_node(node_type type, node_value val, ast_node* parent)
{
    ast_node* node = (ast_node*)calloc(1, sizeof(*node));
    *node = {
        .type = type,
        .value = val,
        .parent = parent,
        .left = NULL,
        .right = NULL
    };

    return node;
}

ast_node* make_binary_node(op_type op, ast_node * left, ast_node * right)
{
    ast_node* node = make_node(T_OP, {.op = op});
    node->left = left;
    node->right = right;
    left->parent = node;
    right->parent = node;
    return node;
}

ast_node * make_unary_node(op_type op, ast_node * right)
{
    ast_node* node = make_node(T_OP, {.op = op});
    node->right = right;
    right->parent = node;
    return node;
}

ast_node * make_number_node(double val)
{
    return make_node(T_NUM, {.num = val});
}

ast_node * make_var_node(size_t id)
{
    return make_node(T_VAR, {.var_id = id});
}

void delete_node(ast_node* node)
{
    LOG_ASSERT(node != NULL, return);
    LOG_ASSERT(node->left == NULL, return);
    LOG_ASSERT(node->right == NULL, return);

    free(node);
}

void delete_subtree(ast_node* node)
{
    LOG_ASSERT(node != NULL, return);

    if (node->left) delete_subtree(node->left);
    if (node->right) delete_subtree(node->right);

    free(node);
}

syntax_tree* tree_ctor(void)
{
    syntax_tree *result = (syntax_tree*) calloc(1, sizeof(*result));
    *result = {
        .root    = NULL,
        .var_cnt = 0,
        .vars    = {}
    };
    return result;
}

void tree_dtor(syntax_tree* tree)
{
    LOG_ASSERT(tree != NULL, return);

    if (tree->root) delete_subtree(tree->root);
    tree->root = NULL;

    for (size_t i = 0; i < tree->var_cnt; i++)
    {
        free(tree->vars[i]);
        tree->vars[i] = NULL;
    }

    free(tree);
}

ast_node* tree_begin(syntax_tree* tree)
{
    LOG_ASSERT(tree != NULL, return NULL);
    LOG_ASSERT(tree->root != NULL, return NULL);

    ast_node* node = tree->root;
    while (node->left)
        node = node->left;
    
    return node;
}

ast_node* next_iterator(ast_node* node)
{
    LOG_ASSERT(node, return NULL);

    if (node->right)
    {
        node = node->right;
        while (node->left)
            node = node->left;
        return node;
    }

    ast_node* parent = node->parent;
    
    while (parent && parent->left != node)
    {
        node = parent;
        parent = node->parent;
    }

    return parent;
}

ast_node* prev_iterator(ast_node* node)
{
    LOG_ASSERT(node, return NULL);

    if (node->left)
    {
        node = node->left;
        while (node->right)
            node = node->right;
        return node;
    }

    ast_node* parent = node->parent;
    while (parent && parent->right != node)
    {
        node = parent;
        parent = node->parent;
    }

    return parent;
}

ast_node* tree_end(syntax_tree* tree)
{
    LOG_ASSERT(tree != NULL, return NULL);
    LOG_ASSERT(tree->root != NULL, return NULL);

    ast_node* node = tree->root;
    while (node->right)
        node = node->right;
    
    return node;
}

static void print_node(
                    const ast_node* node,
                    const syntax_tree* ast,
                    FILE* stream);
static int requires_grouping(const ast_node* parent, const ast_node* child);
static int is_unary(op_type op);
static void print_op(op_type op, FILE* stream);

void print_tree(const syntax_tree * ast, FILE * stream)
{
    print_node(ast->root, ast, stream);
}

static void print_node(
                    const ast_node * node,
                    const syntax_tree * ast,
                    FILE * stream)
{
    if (node->type == T_NUM)
    {
        fprintf(stream, "%g ", node->value.num);
        return;
    }
    if (node->type == T_VAR)
    {
        LOG_ASSERT_ERROR(node->value.var_id < ast->var_cnt,
        {
            fprintf(stream, "[UNKNOWN]");
            return;
        }, "Unknown variable encountered.", NULL);

        fprintf(stream, "%s ", ast->vars[node->value.var_id]);
        return;
    }
    if (node->value.op == OP_DIV)
    {
        fprintf(stream, "\\frac{");
        print_node(node->left, ast, stream);
        fprintf(stream, "}{");
        print_node(node->right, ast, stream);
        fprintf(stream, "} ");
        return;
    }
    
    int group = 0;
    if (!is_unary(node->value.op))
    {
        group = requires_grouping(node, node->left);
        if (group) fprintf(stream, "\\left( ");
        print_node(node->left, ast, stream);
        if (group) fprintf(stream, "\\right) ");
    }

    print_op(node->value.op, stream);

    if (node->value.op == OP_POW)
    {
        fprintf(stream, "{ ");
        print_node(node->right, ast, stream);
        fprintf(stream, "} ");
        return;
    }

    group = requires_grouping(node, node->right);
    if (group) fprintf(stream, "\\left( ");
    print_node(node->right, ast, stream);
    if (group) fprintf(stream, "\\right) ");
}

int requires_grouping(const ast_node * parent, const ast_node * child)
{
    if (child->type != T_OP)
        return 0;
    LOG_ASSERT(parent->type == T_OP, return 0);

    op_type child_op = child->value.op;

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wswitch-enum"
    switch (parent->value.op)
    {
    case OP_ADD:
        return 0;
    case OP_SUB:
        return child == parent->right
            && (child_op == OP_ADD || child_op == OP_SUB);
    case OP_MUL:
    case OP_DIV:
        return child_op == OP_ADD || child_op == OP_SUB;
    case OP_POW:
        return child == parent->left;
    default:
        return child_op == OP_POW || is_unary(child_op);
    }
    #pragma GCC diagnostic pop

    LOG_ASSERT(0 && "Unreachable code", return 0);
}

int is_unary(op_type op)
{
    return !(
        op == OP_ADD || op == OP_SUB ||
        op == OP_MUL || op == OP_DIV ||
        op == OP_POW
    );
}

static void print_op(op_type op, FILE* stream)
{
    switch (op)
    {
        case OP_ADD: fprintf(stream, "+ ");         return;
        case OP_SUB: fprintf(stream, "- ");         return;
        case OP_MUL: fprintf(stream, "\\cdot ");    return;
        case OP_DIV: fprintf(stream, "/ ");         return;
        case OP_POW: fprintf(stream, "^");          return;
        case OP_NEG: fprintf(stream, "-");          return;
        case OP_LN:  fprintf(stream, "\\ln");       return;
        case OP_SQRT:fprintf(stream, "\\sqrt");     return;
        case OP_SIN: fprintf(stream, "\\sin");      return;
        case OP_COS: fprintf(stream, "\\cos");      return;
        case OP_TAN: fprintf(stream, "\\tan");      return;
        case OP_COT: fprintf(stream, "\\cot");      return;
        case OP_ARCSIN: fprintf(stream, "\\arcsin");return;
        case OP_ARCCOS: fprintf(stream, "\\arccos");return;
        case OP_ARCTAN: fprintf(stream, "\\arctan");return;
        case OP_ARCCOT: fprintf(stream, "\\arccot");return;
        default: LOG_ASSERT(0 && "Invalid enum value.", return);
    }
    LOG_ASSERT(0 && "Unreachable code", return);
}

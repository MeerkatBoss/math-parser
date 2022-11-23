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
        .size    = 0,
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

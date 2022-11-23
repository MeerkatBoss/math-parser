#ifndef TREE_H
#define TREE_H

#include <stddef.h>

enum node_type
{
    T_NUM,
    T_VAR,
    T_OP
};
union node_value{
    double num;
    size_t var_id;
    op_type op;
}; 

#define MATH_FUNC(name, ...) OP_##name,
enum op_type
{
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NEG,
    #include "functions.h"
};
#undef MATH_FUNC

struct ast_node
{
    node_type   type;
    node_value  value;

    ast_node*   parent;
    ast_node*   left;
    ast_node*   right;
};

const size_t MAX_VARS = 16;

struct syntax_tree
{
    ast_node* root;
    size_t size;
    size_t var_cnt;
    char *vars[MAX_VARS];
};

/**
 * @brief Create new tree node with specified parent
 * 
 * @param[in] data Data stored in node
 * @param[in] parent Parent node
 * @return Allocated `ast_node` instance
 */
ast_node* make_node(node_type type, node_value val, ast_node* parent = NULL);

/**
 * @brief Delete created node. Free associated resources
 * 
 * @param[inout] node `ast_node` instance to be deleted
 */
void delete_node(ast_node* node);

/**
 * @brief Delete subtree of chosen node. Free associated resources.
 * 
 * @param[inout] node Subtree root
 */
void delete_subtree(ast_node* node);

/**
 * @brief Create `syntax_tree` instance
 * 
 * @return Constructed `syntax_tree` instance
 */
syntax_tree* tree_ctor(void);

/**
 * @brief Destroy `syntax_tree`
 * 
 * @param[inout] tree `syntax_tree` instance to be destroyed
 */
void tree_dtor(syntax_tree* tree);

/**
 * @brief Get iterator to first tree element
 * 
 * @param[in] tree 
 * @return Tree iterator
 */
ast_node* tree_begin(syntax_tree* tree);

/**
 * @brief Get iterator to next tree node
 * 
 * @param[in] node Current tree node
 * @return Tree iterator
 */
ast_node* next_iterator(ast_node *node);

/**
 * @brief Get iterator to previous tree node
 * 
 * @param[in] node Current tree node
 * @return Tree iterator
 */
ast_node* prev_iterator(ast_node *node);

/**
 * @brief Get iterator to last tree element
 * 
 * @param[in] tree 
 * @return Tree iterator 
 */
ast_node* tree_end(syntax_tree* tree);

#endif
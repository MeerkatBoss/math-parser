/**
 * @file ast.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 * @brief Abstract syntax tree struct
 * @version 0.1
 * @date 2022-11-23
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef TREE_H
#define TREE_H

#include <stddef.h>

/**
 * @brief AST node type
 */
enum node_type
{
    T_NUM,
    T_VAR,
    T_OP
};

/**
 * @brief AST node stored value
 */
union node_value{
    double num;
    size_t var_id;
    op_type op;
}; 

#define MATH_FUNC(name, ...) OP_##name,

/**
 * @brief Operation type for AST nodes of type `T_OP`
 */
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

/**
 * @brief Abstract syntax tree node
 */
struct ast_node
{
    /**
     * @brief Node type
     */
    node_type   type;
    /**
     * @brief Stored value
     */
    node_value  value;

    /**
     * @brief Pointer to parent node (used for iteration)
     */
    ast_node*   parent;
    /**
     * @brief Pointer to left child
     */
    ast_node*   left;
    /**
     * @brief Pointer to right child
     */
    ast_node*   right;
};

/**
 * @brief Maximum allowed number of variables in expression
 */
const size_t MAX_VARS = 16;

/**
 * @brief Abstract syntax tree
 */
struct syntax_tree
{
    /**
     * @brief Tree root node
     */
    ast_node*   root;
    /**
     * @brief Number of used variables
     */
    size_t      var_cnt;
    /**
     * @brief Array of variable names
     */
    char*       vars[MAX_VARS];
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
 * @brief Create syntax tree node for binary operation
 * @param[in] op Operation type
 * @param[inout] left Left operand
 * @param[inout] right Right operand
 * @return Created node
 */
ast_node* make_binary_node(op_type op, ast_node* left, ast_node* right);

/**
 * @brief Create syntax tree node for unary operation
 * @param[in] op Operation type
 * @param[inout] right Right operand
 * @return Created node
 */
ast_node* make_unary_node(op_type op, ast_node* right);

/**
 * @brief Create syntax tree node for number constant
 * @param[in] op Operation type
 * @param[in] val Stored value
 * @return Created node
 */
ast_node* make_number_node(double val);

/**
 * @brief Create syntax tree node for variable
 * @param[in] op Operation type
 * @param[in] id Variable id
 * @return Created node
 */
ast_node* make_var_node(size_t id);

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
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

#include "math_utils.h"

/**
 * @brief AST node type
 */
enum node_type
{
    NODE_NUM,
    NODE_VAR,
    NODE_OP
};

#define MATH_FUNC(name, ...) OP_##name,

/**
 * @brief Operation type for AST nodes of type `NODE_OP`
 */
enum op_type
{
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_POW,
    OP_NEG,
    #include "functions.h"
};

#undef MATH_FUNC

/**
 * @brief AST node stored value
 */
union node_value{
    double num;
    size_t var_id;
    op_type op;
}; 

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

/* TODO: docs */

inline double  get_num(ast_node* node)             { return node->value.num; }
inline int     is_num (ast_node* node)             { return node && node->type == NODE_NUM;}
inline int     num_cmp(ast_node* node, double num) { return is_num(node) && compare_double(get_num(node), num); }

inline op_type get_op (ast_node* node)             { return node->value.op; }
inline int     is_op  (ast_node* node)             { return node && node->type == NODE_OP; }
inline int     op_cmp (ast_node* node, op_type op) { return is_op(node) && get_op(node) == op; }

inline size_t  get_var(ast_node* node)             { return node->value.var_id; }
inline int     is_var (ast_node* node)             { return node && node->type == NODE_VAR; }
inline int     var_cmp(ast_node* node, size_t op)  { return is_var(node) && get_var(node) == op; }

typedef ast_node* tree_iterator;

/**
 * @brief Maximum allowed number of variables in expression
 */
const size_t MAX_VARS = 16;

/**
 * @brief Abstract syntax tree
 */
struct abstract_syntax_tree
{
    /**
     * @brief Tree root node
     */
    ast_node*   root;
    /**
     * @brief Number of used variables
     */

    size_t      var_cnt; // TODO: dynamic_array(char*)???
    /**
     * @brief Array of variable names
     */
    char*       vars[MAX_VARS]; // TODO: make dynamic
};

/**
 * @brief Create new tree node with specified parent and no children.
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

/* TODO: docs */ 

ast_node* copy_tree(ast_node* node);

/**
 * @brief Delete ast node. Free associated resources.
 * 
 * @param[inout] node `ast_node` instance to be deleted
 * @warning This function will fail upon attempting to delete node, which has
 * non-`NULL` children.
 */
void delete_node(ast_node* node);

/**
 * @brief Delete subtree of chosen node. Free associated resources.
 * 
 * @param[inout] node Subtree root
 */
void delete_subtree(ast_node* node);

/**
 * @brief Create `abstract_syntax_tree` instance
 * 
 * @return Constructed `abstract_syntax_tree` instance
 */
abstract_syntax_tree* tree_ctor(void);

/**
 * @brief Destroy `abstract_syntax_tree`
 * 
 * @param[inout] tree `abstract_syntax_tree` instance to be destroyed
 */
void tree_dtor(abstract_syntax_tree* tree);

/**
 * @brief Get iterator to first tree element
 * 
 * @param[in] tree 
 * @return Tree iterator
 */
tree_iterator tree_begin(abstract_syntax_tree* tree);

/**
 * @brief Get iterator to next tree node, following in-order traversal.
 * 
 * @param[in] node Current tree node
 * @return Tree iterator
 */
tree_iterator tree_get_next(tree_iterator it);

/**
 * @brief Get iterator to previous tree node
 * 
 * @param[in] node Current tree node
 * @return Tree iterator
 */
tree_iterator tree_get_prev(tree_iterator it);

/**
 * @brief Get iterator to last tree element
 * 
 * @param[in] tree 
 * @return Tree iterator 
 */
tree_iterator tree_end(abstract_syntax_tree* tree);

/**
 * @brief Output tree to specified stream in LaTeX format
 * 
 * @param[in] ast Printed tree
 * @param[in] stream Output stream
 */
void print_tree(const abstract_syntax_tree* ast, FILE* stream);

#endif
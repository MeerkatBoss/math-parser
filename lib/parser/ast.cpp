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

    if (node-> left) delete_subtree(node->left);
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

static void print_node( // TODO: what is this alignment?
                    const ast_node* node,
                    const syntax_tree* ast,
                    const ast_node* dictionary['Z'-'A'+1],
                    // TODO:                   ^~~~~~~~~ make this a constant, please!
                    FILE* stream);
static int requires_grouping(const ast_node* parent, const ast_node* child);
static int is_unary(op_type op);
static void print_op(op_type op, FILE* stream);
static int extract_blocks(const ast_node* node, const ast_node* dictionary['Z'-'A'+1]);
static char get_definition(const ast_node* node, const ast_node* dictionary['Z'-'A'+1]);

void print_tree(const syntax_tree * ast, FILE * stream)
{
    const ast_node* dictionary['Z'-'A'+1] = {}; // TODO: same
    extract_blocks(ast->root, dictionary);

    char short_root = get_definition(ast->root, dictionary);
    if (short_root != '\0') // TODO: proshe napisat vot zdes costil' na meste i vse
        dictionary[short_root - 'A'] = NULL;

    fprintf(stream, "\\begin{equation}\n");
    print_node(ast->root, ast, dictionary, stream);
    fprintf(stream, "\n\\end{equation}\n\n");
    if (dictionary[0]) // TODO: Explain yourself! Bool variable? Leak of extract_blocks algorithm to outer function!
    {
        fprintf(stream, "Where:\n"
                        "\\begin{itemize}\n");
        for (char i = 'A'; i <= 'Z' && dictionary[i - 'A'] != NULL; i++)
        {
            fprintf(stream, "\t\\item $%c = ", i);
            const ast_node* def = dictionary[i - 'A']; // TODO: YESS, YEEEESSS MORE - 'A'
            dictionary[i - 'A'] = NULL;
            print_node(def, ast, dictionary, stream);
            dictionary[i - 'A'] = def;
            fprintf(stream, "$\n");
        }
        fprintf(stream, "\\end{itemize}\n\n");
    }
}

static void print_node(
                    const ast_node * node,
                    const syntax_tree * ast,
                    const ast_node* dictionary['Z'-'A'+1],
                    FILE * stream)
{
    char shorthand = get_definition(node, dictionary);
    if (shorthand != '\0')
    {
        fprintf(stream, "%c ", shorthand);
        return;
    }

    if (node->type == T_NUM) // TODO: switch?
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
    if (node->value.op == OP_DIV /* TODO: long names looooong naaaaameeesss (use them) */)
    {
        fprintf(stream, "\\frac{");
        print_node(node->left, ast, dictionary, stream);
        fprintf(stream, "}{");
        print_node(node->right, ast, dictionary, stream);
        fprintf(stream, "} ");
        return;
    }
    
    int group = 0;
    if (!is_unary(node->value.op))
    {
        group = requires_grouping(node, node->left);
        if (group) fprintf(stream, "\\left( ");
        print_node(node->left, ast, dictionary, stream);
        if (group) fprintf(stream, "\\right) ");
    }

    print_op(node->value.op, stream);

    if (node->value.op == OP_POW || node->value.op == OP_SQRT)
    {
        fprintf(stream, "{");
        print_node(node->right, ast, dictionary, stream);
        fprintf(stream, "} ");
        return;
    }

    group = requires_grouping(node, node->right);
    if (group) fprintf(stream, "\\left( ");

    print_node(node->right, ast, dictionary, stream);

    if (group) fprintf(stream, "\\right) ");
}

static int requires_grouping(const ast_node * parent, const ast_node * child)
{
    if (child->type != T_OP)
        return 0;
    LOG_ASSERT(parent->type == T_OP, return 0);

    op_type child_op = child->value.op;


    // TODO: Clang?????? WHERE IS MY BELOVED CLANG???!!! I CAN'T!!
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
    case OP_SQRT:
        return 0;
    default:
        return child_op == OP_ADD || child_op == OP_SUB;
    }
    #pragma GCC diagnostic pop

    LOG_ASSERT(0 && "Unreachable code", return 0);
}

static int is_unary(op_type op)
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
    { // TODO: alignment is trash, maaaan
        case OP_ADD: fprintf(stream, "+ ");         return;
        case OP_SUB: fprintf(stream, "- ");         return;
        case OP_MUL: fprintf(stream, "\\cdot ");    return;
        case OP_DIV: fprintf(stream, "/ ");         return;
        case OP_POW: fprintf(stream, "^");          return;
        case OP_NEG: fprintf(stream, "-");          return;
        case OP_LN:  fprintf(stream, "\\ln ");      return;
        case OP_SQRT:fprintf(stream, "\\sqrt ");    return;
        case OP_SIN: fprintf(stream, "\\sin ");     return;
        case OP_COS: fprintf(stream, "\\cos ");     return;
        case OP_TAN: fprintf(stream, "\\tan ");     return;
        case OP_COT: fprintf(stream, "\\cot ");     return;
        case OP_ARCSIN: fprintf(stream, "\\arcsin ");return;
        case OP_ARCCOS: fprintf(stream, "\\arccos ");return;
        case OP_ARCTAN: fprintf(stream, "\\arctan ");return;
        case OP_ARCCOT: fprintf(stream, "\\arccot ");return;
        default: LOG_ASSERT(0 && "Invalid enum value.", return);
    }
    LOG_ASSERT(0 && "Unreachable code", return);
}

// TODO:                                                   this is not dictionary! (labeled_subexpression?)
int extract_blocks(const ast_node * node, const ast_node * dictionary['Z'-'A'+ 1])
{ // TODO: label block? namiiiiinnggggg
    const int MAX_SIZE = 24; // TODO: MAX_SIZE of what?
    if (!node) return 0;
    int size = extract_blocks(node->left, dictionary)
                + extract_blocks(node->right, dictionary) + 1;
    if (size > MAX_SIZE)
    {
        int found = 0;
        for (char i = 'A'; i <= 'Z'; i++)
        {
            if (dictionary[i - 'A']) continue;
            dictionary[i - 'A'] = node;
            // TODO:   ^~~~~~~ make a function?
            found = 1;
            break;
        }
        LOG_ASSERT_ERROR(found, return size,
                            "Too many attempted extractions.", NULL);
        return 1;
    }
    return size;
}

// TODO: namiiiiinnggggg, get_label_of_expression?
char get_definition(const ast_node * node, const ast_node * dictionary['Z'-'A'+ 1])
{
    for (char i = 'A'; i <= 'Z' && dictionary[i - 'A']; i++)
        if (dictionary[i - 'A'] == node) return i;
    return '\0';
}

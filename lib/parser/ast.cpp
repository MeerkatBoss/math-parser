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
    ast_node* node = make_node(NODE_OP, {.op = op});
    node->left = left;
    node->right = right;
    left->parent = node;
    right->parent = node;
    return node;
}

ast_node * make_unary_node(op_type op, ast_node * right)
{
    ast_node* node = make_node(NODE_OP, {.op = op});
    node->right = right;
    right->parent = node;
    return node;
}

ast_node * make_number_node(double val)
{
    return make_node(NODE_NUM, {.num = val});
}

ast_node * make_var_node(var_name var) 
{
    return make_node(NODE_VAR, {.var = var});
}

ast_node* copy_subtree(ast_node * node)
{
    if (!node) return NULL;

    ast_node* res = make_node(node->type, node->value);
    res->left   = copy_subtree(node->left);
    res->right  = copy_subtree(node->right);
    if (res-> left) res-> left->parent = res;
    if (res->right) res->right->parent = res;

    return res;
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

abstract_syntax_tree* tree_ctor(void)
{
    abstract_syntax_tree* result = (abstract_syntax_tree*) calloc(1, sizeof(*result));
    result->root = NULL;
    array_ctor(&result->variables);
    return result;
}

abstract_syntax_tree* tree_copy(const abstract_syntax_tree* src)
{
    abstract_syntax_tree* result = (abstract_syntax_tree*) calloc(1, sizeof(*result));
    result->root = NULL;
    array_copy(&result->variables, &src->variables);
    return result;
}

void tree_dtor(abstract_syntax_tree *tree)
{
    LOG_ASSERT(tree != NULL, return);

    if (tree->root) delete_subtree(tree->root);
    tree->root = NULL;

    array_dtor(&tree->variables);

    free(tree);
}

tree_iterator tree_begin(abstract_syntax_tree *tree)
{
    LOG_ASSERT(tree != NULL, return NULL);
    LOG_ASSERT(tree->root != NULL, return NULL);

    tree_iterator node = tree->root;
    while (node->left)
        node = node->left;
    
    return node;
}

tree_iterator tree_get_next(tree_iterator node)
{
    LOG_ASSERT(node, return NULL);

    if (node->right)
    {
        node = node->right;
        while (node->left)
            node = node->left;
        return node;
    }

    tree_iterator parent = node->parent;
    
    while (parent && parent->left != node)
    {
        node = parent;
        parent = node->parent;
    }

    return parent;
}

tree_iterator tree_get_prev(tree_iterator node)
{
    LOG_ASSERT(node, return NULL);

    if (node->left)
    {
        node = node->left;
        while (node->right)
            node = node->right;
        return node;
    }

    tree_iterator parent = node->parent;
    while (parent && parent->right != node)
    {
        node = parent;
        parent = node->parent;
    }

    return parent;
}

tree_iterator tree_end(abstract_syntax_tree* tree)
{
    LOG_ASSERT(tree != NULL, return NULL);
    LOG_ASSERT(tree->root != NULL, return NULL);

    tree_iterator node = tree->root;
    while (node->right)
        node = node->right;
    
    return node;
}

void plot_node(const ast_node *node, FILE *output)
{
    #define PLOT_INFIX(op) do\
        {\
            fputc('(', output); plot_node(node->left, output);\
            fputs(")" #op "(", output); plot_node(node->right, output); fputc(')', output);\
        } while(0)
    #define PLOT_UNARY(op) do\
        {\
            fputs(#op "(", output); plot_node(node->right, output); fputc(')', output);\
        } while(0)
    #define PLOT_COMPOUND(op, plot) do\
        {\
            fputs("(" #op "(", output); plot; fputs("))", output);\
        } while(0)
    
    if (is_num(node)) { fprintf(output, "%g", get_num(node)); return; }
    if (is_var(node)) { fprintf(output, "%s", get_var(node)); return; }

    switch(get_op(node))
    {
    case OP_ADD:    PLOT_INFIX(+);                          break;
    case OP_SUB:    PLOT_INFIX(-);                          break;
    case OP_MUL:    PLOT_INFIX(*);                          break;
    case OP_DIV:    PLOT_INFIX(/);                          break;
    case OP_NEG:    PLOT_UNARY(-);                          break;
    case OP_LN:     PLOT_UNARY(log);                        break;
    case OP_SQRT:   PLOT_UNARY(sqrt);                       break;
    case OP_SIN:    PLOT_UNARY(sin);                        break;
    case OP_COS:    PLOT_UNARY(cos);                        break;
    case OP_TAN:    PLOT_UNARY(tan);                        break;
    case OP_COT:    PLOT_COMPOUND(1/, PLOT_UNARY(tan));     break;
    case OP_ARCSIN: PLOT_UNARY(asin);                       break;
    case OP_ARCCOS: PLOT_COMPOUND(pi/2-, PLOT_UNARY(asin)); break;
    case OP_ARCTAN: PLOT_UNARY(atan);                       break;
    case OP_ARCCOT: PLOT_COMPOUND(pi/2, PLOT_UNARY(atan));  break;
    case OP_POW:    PLOT_INFIX(**);                         break;
    }

    #undef PLOT_INFIX
    #undef PLOT_UNARY
    #undef PLOT_COMPOUND
}

void plot_tangent(const ast_node *node, const ast_node* tangent, const char *filename)
{
    FILE* plot = popen("gnuplot", "w");

    fputs("set terminal png\n", plot);
    fprintf(plot, "set output \"%s\"\n", filename);
    fputs("set grid\n", plot);

    fputs("plot ", plot);
    plot_node(node, plot);
    fputs(" title \"y = f(x)\", ", plot);
    plot_node(tangent, plot);
    fputs(" title \"tangent\"\n", plot);

    pclose(plot);
}

static const int MAX_LABELS = 'Z'-'A'+1;


static void print_subtree(
                        const ast_node* node,
                        const ast_node* labels[MAX_LABELS],
                        string_builder* builder);
static int requires_grouping(const ast_node* parent, const ast_node* child);
static int is_unary(op_type op);
static void print_op(op_type op, string_builder* builder);
static int label_subtrees(const ast_node* node, const ast_node* labels[MAX_LABELS]);
static char find_label(const ast_node* node, const ast_node* labels[MAX_LABELS]);
void print_labels(const ast_node * labels[MAX_LABELS], string_builder* builder);
static int add_label(const ast_node* node, const ast_node* labels[MAX_LABELS]);
static inline int has_labels(const ast_node* labels[MAX_LABELS])
{
    return labels[0] != NULL;
}
static inline const ast_node* get_node(
                                        char label,
                                        const ast_node* labels[MAX_LABELS])
{
    return labels[label - 'A'];
}
static inline void set_node(
                            char label,
                            const ast_node* node,
                            const ast_node* labels[MAX_LABELS])
{
    labels[label - 'A'] = node;
}

void print_node(const ast_node* root, string_builder * builder)
{
    const ast_node* labels[MAX_LABELS] = {};
    label_subtrees(root, labels);

    string_builder_append_format(builder, "\\begin{equation}\n");
    print_subtree(root, labels, builder);
    string_builder_append_format(builder, "\n\\end{equation}\n");
    if (has_labels(labels))
    {
        string_builder_append_format(builder, "Where:\n"
                        "\\begin{itemize}\n");
        print_labels(labels, builder);
        string_builder_append_format(builder, "\\end{itemize}\n\n");
    }
}

static void print_subtree(
                        const ast_node * node,
                        const ast_node* labels[MAX_LABELS],
                        string_builder * builder)
{
    char shorthand = find_label(node, labels);
    if (shorthand != '\0')
    {
        string_builder_append_format(builder, "%c ", shorthand);
        return;
    }

    if (is_num(node))
    {
        string_builder_append_format(builder, "%g ", get_num(node));
        return;
    }
    if (is_var(node))
    {
        string_builder_append_format(builder, "%s ", get_var(node));
        return;
    }

    if (op_cmp(node, OP_DIV))
    {
        string_builder_append_format(builder, "\\frac{");
        print_subtree(node->left, labels, builder);
        string_builder_append_format(builder, "}{");
        print_subtree(node->right, labels, builder);
        string_builder_append_format(builder, "} ");
        return;
    }
    
    int group = 0;
    if (!is_unary(node->value.op))
    {
        group = requires_grouping(node, node->left);
        if (group) string_builder_append_format(builder, "\\left( ");
        print_subtree(node->left, labels, builder);
        if (group) string_builder_append_format(builder, "\\right) ");
    }

    print_op(get_op(node), builder);

    if (op_cmp(node, OP_POW) || op_cmp(node, OP_SQRT))
    {
        string_builder_append_format(builder, "{");
        print_subtree(node->right, labels, builder);
        string_builder_append_format(builder, "} ");
        return;
    }

    group = requires_grouping(node, node->right);
    if (group) string_builder_append_format(builder, "\\left( ");

    print_subtree(node->right, labels, builder);

    if (group) string_builder_append_format(builder, "\\right) ");
}

static int requires_grouping(const ast_node * parent, const ast_node * child)
{
    if (child->type != NODE_OP)
        return 0;
    LOG_ASSERT(parent->type == NODE_OP, return 0);

    op_type child_op = get_op(child);

    switch (get_op(parent))
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

static void print_op(op_type op, string_builder* builder)
{
    switch (op)
    {
        case OP_ADD:    string_builder_append_format(builder, "+ ");          return;
        case OP_SUB:    string_builder_append_format(builder, "- ");          return;
        case OP_MUL:    string_builder_append_format(builder, "\\cdot ");     return;
        case OP_DIV:    string_builder_append_format(builder, "/ ");          return;
        case OP_POW:    string_builder_append_format(builder, "^");           return;
        case OP_NEG:    string_builder_append_format(builder, "-");           return;
        case OP_LN:     string_builder_append_format(builder, "\\ln ");       return;
        case OP_SQRT:   string_builder_append_format(builder, "\\sqrt ");     return;
        case OP_SIN:    string_builder_append_format(builder, "\\sin ");      return;
        case OP_COS:    string_builder_append_format(builder, "\\cos ");      return;
        case OP_TAN:    string_builder_append_format(builder, "\\tan ");      return;
        case OP_COT:    string_builder_append_format(builder, "\\cot ");      return;
        case OP_ARCSIN: string_builder_append_format(builder, "\\arcsin ");   return;
        case OP_ARCCOS: string_builder_append_format(builder, "\\arccos ");   return;
        case OP_ARCTAN: string_builder_append_format(builder, "\\arctan ");   return;
        case OP_ARCCOT: string_builder_append_format(builder, "\\arccot ");   return;
        default: LOG_ASSERT(0 && "Invalid enum value.", return);
    }
    LOG_ASSERT(0 && "Unreachable code", return);
}

int label_subtrees(const ast_node * node, const ast_node * labels[MAX_LABELS])
{
    const int MAX_TREE_SIZE = 24;
    if (!node) return 0;
    int left_size  = label_subtrees(node-> left, labels);
    int right_size = label_subtrees(node->right, labels);
    if (left_size  > MAX_TREE_SIZE)
    {
        add_label(node-> left, labels);
        left_size  = 1;
    }
    if (right_size > MAX_TREE_SIZE)
    {
        add_label(node->right, labels);
        right_size = 1;
    }
    
    return left_size + 1 + right_size;
}

char find_label(const ast_node * node, const ast_node * labels[MAX_LABELS])
{
    for (char i = 'A'; i <= 'Z' && labels[i - 'A']; i++)
        if (labels[i - 'A'] == node) return i;
    return '\0';
}

void print_labels(const ast_node * labels[MAX_LABELS], string_builder* builder)
{
    for (char i = 'A'; i <= 'Z' && get_node(i, labels) != NULL; i++)
    {
        string_builder_append_format(builder, "\t\\item $%c = ", i);
        const ast_node* def = get_node(i, labels);
        set_node(i, NULL, labels);
        print_subtree(def, labels, builder);
        set_node(i, def, labels);
        string_builder_append_format(builder, "$\n");
    }
}

int add_label(const ast_node * node, const ast_node * labels[MAX_LABELS])
{
    int found = 0;
    for (char i = 'A'; i <= 'Z'; i++)
    {
        if (get_node(i, labels) != NULL) continue;
        set_node(i, node, labels);
        found = 1;
        break;
    }
    LOG_ASSERT_ERROR(found, return -1,
                        "Too many attempted extractions.", NULL);
    return 0;
}

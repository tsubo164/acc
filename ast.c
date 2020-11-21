#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

#define TERMINAL_COLOR_BLACK   "\x1b[30m"
#define TERMINAL_COLOR_RED     "\x1b[31m"
#define TERMINAL_COLOR_GREEN   "\x1b[32m"
#define TERMINAL_COLOR_YELLOW  "\x1b[33m"
#define TERMINAL_COLOR_BLUE    "\x1b[34m"
#define TERMINAL_COLOR_MAGENTA "\x1b[35m"
#define TERMINAL_COLOR_CYAN    "\x1b[36m"
#define TERMINAL_COLOR_WHITE   "\x1b[37m"
#define TERMINAL_COLOR_RESET   "\x1b[39m"

#define TERMINAL_DECORATION_BOLD    "\x1b[1m"
#define TERMINAL_DECORATION_RESET   "\x1b[0m"

/*
static const struct data_type *promote_data_type(
        const struct ast_node *n1, const struct ast_node *n2)
{
    if (!n1 && !n2) {
        return type_void();
    }

    if (!n1) {
        return n2->dtype;
    }

    if (!n2) {
        return n1->dtype;
    }

    if (n1->dtype->kind > n2->dtype->kind) {
        return n1->dtype;
    } else {
        return n2->dtype;
    }
}
*/

struct ast_node *new_ast_node(enum ast_node_kind kind,
        struct ast_node *l, struct ast_node *r)
{
    struct ast_node *n = malloc(sizeof(struct ast_node));
    n->kind = kind;
    n->l = l;
    n->r = r;

    /*
    n->dtype = promote_data_type(l, r);
    */
    n->dtype = type_void();

    n->data.ival = 0;

    return n;
}

void free_ast_node(struct ast_node *node)
{
    if (!node) {
        return;
    }

    free_ast_node(node->l);
    free_ast_node(node->r);
}

void ast_node_set_symbol(struct ast_node *node, const struct symbol *sym)
{
    if (!node || !sym) {
        return;
    }

    node->data.sym = sym;
    node->dtype = sym->dtype;
}

#define AST_NODE_LIST(N) \
    N(NOD_LIST) \
    N(NOD_GLOBAL) \
    N(NOD_COMPOUND) \
    N(NOD_STMT) \
    N(NOD_EXT) \
    N(NOD_IF) \
    N(NOD_RETURN) \
    N(NOD_WHILE) \
    N(NOD_ASSIGN) \
    N(NOD_STRUCT_DECL) \
    N(NOD_MEMBER_DECL) \
    N(NOD_STRUCT_REF) \
    N(NOD_VAR) \
    N(NOD_GLOBAL_VAR) \
    N(NOD_VAR_DEF) \
    N(NOD_ADDR) \
    N(NOD_DEREF) \
    N(NOD_CALL) \
    N(NOD_FUNC_DEF) \
    N(NOD_FUNC_BODY) \
    N(NOD_ARG) \
    N(NOD_PARAM) \
    N(NOD_PARAM_DEF) \
    N(NOD_NUM) \
    N(NOD_ADD) \
    N(NOD_SUB) \
    N(NOD_MUL) \
    N(NOD_DIV) \
    N(NOD_LT) \
    N(NOD_GT) \
    N(NOD_LE) \
    N(NOD_GE) \
    N(NOD_EQ) \
    N(NOD_NE) \
    N(NOD_IDENT) \
    N(NOD_DECL) \
    N(NOD_DECLARATOR) \
    N(NOD_DIRECT_DECL) \
    N(NOD_DECL_INIT) \
    N(NOD_DECL_IDENT) \
    N(NOD_DECL_FUNC) \
    N(NOD_DECL_PARAM) \
    N(NOD_TYPE_CHAR) \
    N(NOD_TYPE_INT) \
    N(NOD_TYPE_POINTER) \
    N(NOD_TYPE_ARRAY) \
    N(NOD_TYPE_STRUCT)

const char *node_to_string(const struct ast_node *node)
{
    if (node == NULL) {
        return "null";
    }

#define N(kind) case kind: return #kind;
    switch (node->kind) {
AST_NODE_LIST(N)
    default: return "**unknown**";
    }
#undef N
}

static void print_tree_recursive(const struct ast_node *tree, int depth)
{
    /*
    if (!tree)
        return;
    */

    {
        int i;
        for (i = 0; i < depth; i++) {
            /*
            printf("|   ");
            */
            printf("  ");
        }
    }

    if (!tree) {
        printf("(null)\n");
        /*
        printf(".\n");
        */
        return;
    }

#if 0
    if (tree->data.sym != NULL) {
        printf(TERMINAL_COLOR_CYAN);
        printf(TERMINAL_DECORATION_BOLD);
            printf("%s", node_to_string(tree));
        printf(TERMINAL_DECORATION_RESET);
        printf(TERMINAL_COLOR_RESET);
    } else {
        printf("%s", node_to_string(tree));
    }
#endif
    switch (tree->kind) {
    case NOD_DECL:
    case NOD_DECL_PARAM:
    case NOD_DECL_IDENT:
    case NOD_IDENT:
    case NOD_NUM:
        printf(TERMINAL_COLOR_CYAN);
        printf(TERMINAL_DECORATION_BOLD);
            printf("%s", node_to_string(tree));
        printf(TERMINAL_DECORATION_RESET);
        printf(TERMINAL_COLOR_RESET);
        break;

    case NOD_TYPE_CHAR:
    case NOD_TYPE_INT:
    case NOD_TYPE_POINTER:
    case NOD_TYPE_ARRAY:
    case NOD_TYPE_STRUCT:
        printf(TERMINAL_COLOR_RED);
        printf(TERMINAL_DECORATION_BOLD);
            printf("%s", node_to_string(tree));
        printf(TERMINAL_DECORATION_RESET);
        printf(TERMINAL_COLOR_RESET);
        break;

    default:
        printf("%s", node_to_string(tree));
        break;
    }

    switch (tree->kind) {
        /*
    case NOD_STRUCT_DECL:
    case NOD_FUNC_DEF:
    case NOD_PARAM_DEF:
    case NOD_PARAM:
    case NOD_VAR_DEF:
    case NOD_VAR:
        */

        /*
    case NOD_DECL:
    case NOD_DECL_PARAM:
        */
    case NOD_DECL_IDENT:
    case NOD_IDENT:
        /*
        printf(" (%s)", tree->data.sym->name);
        printf(" => ");
        */
        printf(" ");
        printf(TERMINAL_COLOR_GREEN);
        printf(TERMINAL_DECORATION_BOLD);
        /* XXX */
            printf("%s", tree->sval);
        /*
        if (tree->data.sym)
            printf("%s", tree->data.sym->name);
        else
            printf("%s", tree->sval);
        */
        printf(TERMINAL_DECORATION_RESET);
        printf(TERMINAL_COLOR_RESET);
        break;

    case NOD_TYPE_STRUCT:
        printf(TERMINAL_COLOR_BLUE);
        printf(TERMINAL_DECORATION_BOLD);
            printf(" %s", tree->sval);
        printf(TERMINAL_DECORATION_RESET);
        printf(TERMINAL_COLOR_RESET);
        break;

    case NOD_NUM:
        printf(TERMINAL_COLOR_MAGENTA);
        printf(TERMINAL_DECORATION_BOLD);
            printf(" %d", tree->data.ival);
        printf(TERMINAL_DECORATION_RESET);
        printf(TERMINAL_COLOR_RESET);
        break;

    default:
        break;
    }
    printf("\n");

    print_tree_recursive(tree->l, depth + 1);
    print_tree_recursive(tree->r, depth + 1);
}

void print_tree(const struct ast_node *tree)
{
    print_tree_recursive(tree, 0);
}

static void print_decl_recursive(const struct ast_node *tree)
{
    if (!tree)
        return;

    switch (tree->kind) {

    case NOD_DECL:
        printf("declaration: ");
        break;

    case NOD_DECL_FUNC:
        /*
        printf(" function returning");
        break;
        */
        printf(" function(");
        print_decl_recursive(tree->l);
        printf(") returning");
        print_decl_recursive(tree->r);
        return;

    case NOD_TYPE_ARRAY:
        if (tree->data.ival > 0)
            printf(" array %d of", tree->data.ival);
        else
            printf(" array of");
        break;

    case NOD_TYPE_POINTER:
        printf(" pointer to");
        break;

    case NOD_TYPE_CHAR:
        printf(" char");
        break;

    case NOD_TYPE_INT:
        printf(" int");
        break;

    case NOD_TYPE_STRUCT:
        printf(" struct %s", tree->sval);
        break;

    case NOD_DECL_IDENT:
        printf("%s is", tree->sval);
        break;

    default:
        break;
    }

    print_decl_recursive(tree->l);
    print_decl_recursive(tree->r);
}

void print_decl(const struct ast_node *tree)
{
    print_decl_recursive(tree);
    printf("\n");
}

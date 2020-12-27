#include <stdio.h>
#include <string.h>
#include "semantics.h"
#include "message.h"
#include "ast.h"

static int eval_(const struct ast_node *node, struct message_list *messages)
{
    int l, r;

    if (!node)
        return 0;

    switch (node->kind) {

    case NOD_ADD:
        l = eval_(node->l, messages);
        r = eval_(node->r, messages);
        return l + r;

    case NOD_SUB:
        l = eval_(node->l, messages);
        r = eval_(node->r, messages);
        return l - r;

    case NOD_MUL:
        l = eval_(node->l, messages);
        r = eval_(node->r, messages);
        return l * r;

    case NOD_DIV:
        l = eval_(node->l, messages);
        r = eval_(node->r, messages);
        return l / r;

    case NOD_NUM:
        return node->ival;

    case NOD_DECL_IDENT:
    case NOD_IDENT:
        if (node->sym->kind != SYM_ENUMERATOR) {
            add_error(messages, "expression is not a constant expression", 0);
            return 0;
        }
        return node->sym->mem_offset;

    default:
        add_error(messages, "expression is not a constant expression", 0);
        return 0;
    }
}

static const struct data_type *promote_data_type(
        const struct ast_node *n1, const struct ast_node *n2)
{
    if (!n1 && !n2) {
        return type_void();
    }

    if (!n1) {
        return n2->type;
    }

    if (!n2) {
        return n1->type;
    }

    if (n1->type->kind > n2->type->kind) {
        return n1->type;
    } else {
        return n2->type;
    }
}

static int align_to(int pos, int align)
{
    return ((pos + align - 1) / align) * align;
}

static void compute_struct_size(struct symbol_table *table, struct symbol *strc)
{
    struct symbol *sym = strc;
    int total_offset = 0;

    for (;;) {
        /* TODO support nested struct by checking scope level */
        if (sym->kind == SYM_MEMBER) {
            const int size  = sym->type->byte_size;
            const int align = sym->type->alignment;
            const int len   = sym->type->array_len;

            total_offset = align_to(total_offset, align);
            sym->mem_offset = total_offset;
            total_offset += len * size;
        }

        /* TODO struct scope_level is NOT ALWAYS 1 */
        if (sym->kind == SYM_SCOPE_END && sym->scope_level == 1) {
            break;
        }
        sym++;
    }

    strc->type->byte_size = align_to(total_offset, strc->type->alignment);
    strc->mem_offset = strc->type->byte_size;
}

static void compute_func_stack_size(struct symbol_table *table, struct symbol *func)
{
    struct symbol *sym = func;
    int total_offset = 0;

    for (;;) {
        if (is_param(sym) || is_local_var(sym)) {
            int size  = 0;
            int align = 0;
            int len   = 0;

            if (sym->type->kind == DATA_TYPE_STRUCT) {
                struct symbol *strc = lookup_symbol(table, sym->type->tag, SYM_TAG_STRUCT);
                size  = strc->type->byte_size;
                align = strc->type->alignment;
                len   = strc->type->array_len;
            } else {
                size  = sym->type->byte_size;
                align = sym->type->alignment;
                len   = sym->type->array_len;
            }

            total_offset = align_to(total_offset, align);
            total_offset += len * size;
            sym->mem_offset = total_offset;
        }

        if (sym->kind == SYM_TAG_STRUCT) {
            compute_struct_size(table, sym);
        }

        if (sym->kind == SYM_SCOPE_END && sym->scope_level == 1) {
            break;
        }
        sym++;
    }

    func->mem_offset = align_to(total_offset, 16);
}

static void allocate_local_storage(struct symbol_table *table)
{
    const int N = get_symbol_count(table);
    int i;

    for (i = 0; i < N; i++) {
        struct symbol *sym = get_symbol(table, i);

        if (sym->kind == SYM_FUNC) {
            compute_func_stack_size(table, sym);
        }

        if (sym->kind == SYM_TAG_STRUCT) {
            compute_struct_size(table, sym);
        }
    }
}

static int analyze_symbol_usage(struct symbol_table *table, struct message_list *messages)
{
    const int N = get_symbol_count(table);
    int i;

    for (i = 0; i < N; i++) {
        const struct symbol *sym = get_symbol(table, i);
        /* TODO remove this */
        const struct position pos = {0};

        if (is_local_var(sym)) {
            if (sym->is_redefined)
                add_error(messages, "redefinition of variable", &pos);

            if (!sym->is_defined && sym->is_used)
                add_error(messages, "use of undefined symbol", &pos);

            if (sym->is_defined && !sym->is_used)
                add_warning(messages, "unused variable", &pos);

            if (sym->is_defined && sym->is_used && !sym->is_initialized)
                add_warning(messages, "uninitialized variable used", &pos);
        }
        else if (is_enumerator(sym)) {
            if (!sym->is_defined && sym->is_used)
                add_error(messages, "use of undefined symbol", &pos);

            if (sym->is_assigned)
                add_error(messages, "expression is not assignable", &pos);
        }
        else if (is_func(sym)) {
            if (!sym->is_defined && sym->is_used)
                add_warning(messages, "implicit declaration of function", &pos);
        }
    }
    return 0;
}

enum decl_kind {
    DECL_VAR,
    DECL_FUNC,
    DECL_PARAM,
    DECL_STRUCT,
    DECL_MEMBER
};

struct declaration {
    int kind;
    const char *ident;
    struct data_type *type;
    int has_init;
    int is_lvalue;
};

static void duplicate_decl(struct declaration *dest, const struct declaration *src)
{

    /* TODO make duplicate_type in data_type.c */
    *dest = *src;
}

static void enum_val_(struct ast_node *tree, int *pval, struct message_list *messages)
{
    if (!tree)
        return;

    switch (tree->kind) {

    case NOD_SPEC_ENUM:
        *pval = 0;
        enum_val_(tree->r, pval, messages);
        return;

    case NOD_DECL_ENUMERATOR:
        {
            /* TODO remove const cast */
            struct symbol *sym = (struct symbol *) tree->l->sym;

            if (tree->r)
                *pval = eval_(tree->r, messages);

            sym->mem_offset = *pval;
            *pval = *pval + 1;
        }
        return;

    default:
        enum_val_(tree->l, pval, messages);
        enum_val_(tree->r, pval, messages);
        return;
    }
}

static void compute_enum_values(struct ast_node *tree, struct message_list *messages)
{
    int val = 0;
    enum_val_(tree, &val, messages);
}

static void check_control_for(struct ast_node *tree, struct message_list *messages)
{
    if (!tree)
        return;

    switch (tree->kind) {

    case NOD_FOR:
        check_control_for(tree->l, messages);
        return;

    case NOD_FOR_PRE_COND:
        if (!tree->r) {
            tree->r = new_ast_node(NOD_NUM, NULL, NULL);
            tree->r->ival = 1;
        }
        return;

    default:
        check_control_for(tree->l, messages);
        check_control_for(tree->r, messages);
        return;
    }
}

static void check_init_(struct ast_node *tree,
        struct symbol_table *table, struct declaration *decl)
{
    if (!tree)
        return;

    switch (tree->kind) {

    case NOD_DECL_INIT:
        {
            struct declaration new_decl = {0};
            duplicate_decl(&new_decl, decl);
            new_decl.has_init = tree->r ? 1 : 0;
            if (new_decl.has_init) {
                check_init_(tree->l, table, &new_decl);
                check_init_(tree->r, table, decl);
            }
            return;
        }

    case NOD_DECL_IDENT:
        if (decl->has_init) {
            /* TODO remove const cast */
            struct symbol *sym = (struct symbol *) tree->sym;
            sym->is_initialized = 1;
        }
        return;

    case NOD_ASSIGN:
        {
            struct declaration new_decl = {0};
            /* evaluate rvalue first to check a = a + 1; */
            check_init_(tree->r, table, decl);
            new_decl.is_lvalue = 1;
            check_init_(tree->l, table, &new_decl);
            return;
        }

    case NOD_IDENT:
        if (decl->is_lvalue && !tree->sym->is_used) {
            /* TODO remove const cast */
            struct symbol *sym = (struct symbol *) tree->sym;
            sym->is_initialized = 1;
        }
        if (decl->is_lvalue) {
            /* TODO remove const cast */
            struct symbol *sym = (struct symbol *) tree->sym;
            sym->is_assigned = 1;
        }
        {
            /* TODO remove const cast */
            struct symbol *sym = (struct symbol *) tree->sym;
            sym->is_used = 1;
        }
        return;

    default:
        break;
    }

    check_init_(tree->l, table, decl);
    check_init_(tree->r, table, decl);
}

static void check_initialization(struct ast_node *tree, struct symbol_table *table)
{
    struct declaration decl = {0};
    check_init_(tree, table, &decl);
    return;
}

static void add_types(struct ast_node *node, struct symbol_table *table)
{
    if (!node)
        return;

    add_types(node->l, table);
    add_types(node->r, table);

    switch (node->kind) {

    case NOD_ASSIGN:
    case NOD_ADD_ASSIGN:
    case NOD_DECL_INIT:
        node->type = node->l->type;
        break;

    case NOD_DEREF:
        node->type = promote_data_type(node->l, node->r);
        node->type = node->type->ptr_to;;
        break;

    case NOD_CALL:
        node->type = node->l->type;
        break;

    case NOD_STRUCT_REF:
        node->type = node->sym->type;
        break;

    /* nodes with symbol */
    case NOD_DECL_IDENT:
    case NOD_IDENT:
        node->type = node->sym->type;
        break;

    /* nodes with literal */
    case NOD_NUM:
        node->type = type_int();
        break;

    case NOD_STRING:
        node->type = type_ptr(type_char());
        break;

    default:
        node->type = promote_data_type(node->l, node->r);
        break;
    }
}

int semantic_analysis(struct ast_node *tree,
        struct symbol_table *table, struct message_list *messages)
{
    check_initialization(tree, table);
    check_control_for(tree, messages);
    compute_enum_values(tree, messages);

    analyze_symbol_usage(table, messages);

    add_types(tree, table);
    allocate_local_storage(table);

    return 0;
}

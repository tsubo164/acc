#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gen_x86.h"
#include "type.h"

static int att_syntax = 1;

enum data_tag {
    VARI = -1, /* variable based on context */
    BYTE = 0,
    WORD,
    LONG,
    QUAD
};

struct data_spec {
    const char *suffix;
    const char *directive;
    const char *sizename;
};

const struct data_spec data_spec_table[] = {
    {"b", "byte  ptr", "byte"},
    {"w", "word  ptr", "word"},
    {"l", "dword ptr", "long"},
    {"q", "qword ptr", "quad"}
};

static const char *get_data_suffix(int tag)
{
    return data_spec_table[tag].suffix;
}

static const char *get_data_directive(int tag)
{
    return data_spec_table[tag].directive;
}

static const char *get_data_name(int tag)
{
    return data_spec_table[tag].sizename;
}

/* register tables */
static const char *A__[]  = {"al",  "ax", "eax", "rax"};
static const char *B__[]  = {"bl",  "bx", "ebx", "rbx"};
static const char *C__[]  = {"cl",  "cx", "ecx", "rcx"};
static const char *D__[]  = {"dl",  "dx", "edx", "rdx"};
static const char *SI__[] = {"sil", "si", "esi", "rsi"};
static const char *DI__[] = {"dil", "di", "edi", "rdi"};
static const char *IP__[] = {"ipl", "ip", "eip", "rip"};
static const char *BP__[] = {"bpl", "bp", "ebp", "rbp"};
static const char *SP__[] = {"spl", "sp", "esp", "rsp"};
static const char *R8__[] = {"r8b", "r8w", "r8d", "r8"};
static const char *R9__[] = {"r9b", "r9w", "r9d", "r9"};
static const char **ARG_REG__[] = {DI__, SI__, D__, C__, R8__, R9__};

struct opecode {
    const char *mnemonic;
    int has_suffix;
};

/* opecodes */
const struct opecode MOV_   = {"mov",   1};
const struct opecode ADD_   = {"add",   1};
const struct opecode SUB_   = {"sub",   1};
const struct opecode IMUL_  = {"imul",  1};
const struct opecode DIV_   = {"div",   1};
const struct opecode IDIV_  = {"idiv",  1};
const struct opecode XOR_   = {"xor",   1};
const struct opecode CMP_   = {"cmp",   1};
const struct opecode POP_   = {"pop",   0};
const struct opecode PUSH_  = {"push",  0};
const struct opecode CALL_  = {"call",  0};
const struct opecode LEA_   = {"lea",   0};
const struct opecode RET_   = {"ret",   0};
const struct opecode MOVSB_ = {"movsb", 1};
const struct opecode MOVSW_ = {"movsw", 1};
const struct opecode MOVSL_ = {"movsl", 1};
const struct opecode MOVZB_ = {"movzb", 1};
const struct opecode MOVZW_ = {"movzw", 1};

const struct opecode JE_    = {"je",  0};
const struct opecode JNE_   = {"jne", 0};
const struct opecode JMP_   = {"jmp", 0};

const struct opecode SETE_  = {"sete",  0};
const struct opecode SETNE_ = {"setne", 0};
const struct opecode SETL_  = {"setl",  0};
const struct opecode SETG_  = {"setg",  0};
const struct opecode SETLE_ = {"setle", 0};
const struct opecode SETGE_ = {"setge", 0};
const struct opecode CLTD_  = {"cltd",  0};
const struct opecode CQTO_  = {"cqto",  0};

enum operand_kind {
    OPR_NONE,
    OPR_REG,
    OPR_ADDR,
    OPR_IMME,
    OPR_LABEL,
    OPR_LABEL__,
    OPR_STR
};

struct operand {
    int kind;
    int data_tag;
    const char **reg_table;
    const char *string;
    long immediate;
    int disp;
    int label_id;
    int block_id;
};
#define INIT_OPERAND {OPR_NONE, VARI}

/* variable name registers */
const struct operand A_  = {OPR_REG, VARI, A__};
const struct operand B_  = {OPR_REG, VARI, B__};
const struct operand C_  = {OPR_REG, VARI, C__};
const struct operand D_  = {OPR_REG, VARI, D__};
const struct operand SI_ = {OPR_REG, VARI, SI__};
const struct operand DI_ = {OPR_REG, VARI, DI__};
const struct operand IP_ = {OPR_REG, VARI, IP__};
const struct operand BP_ = {OPR_REG, VARI, BP__};
const struct operand SP_ = {OPR_REG, VARI, SP__};

/* fixed name registers */
const struct operand AL  = {OPR_REG, BYTE, A__};
const struct operand AX  = {OPR_REG, WORD, A__};
const struct operand EAX = {OPR_REG, LONG, A__};

const struct operand RAX = {OPR_REG, QUAD, A__};
const struct operand RDX = {OPR_REG, QUAD, D__};
const struct operand RSI = {OPR_REG, QUAD, SI__};
const struct operand RIP = {OPR_REG, QUAD, IP__};
const struct operand RBP = {OPR_REG, QUAD, BP__};
const struct operand RSP = {OPR_REG, QUAD, SP__};

/* 2, 0x8, ... */
struct operand imme(long value)
{
    struct operand o = INIT_OPERAND;
    o.kind = OPR_IMME;
    o.immediate = value;

    return o;
}

/* (base) */
struct operand addr1(struct operand oper)
{
    struct operand o = oper;
    o.kind = OPR_ADDR;
    o.disp = 0;

    return o;
}

/* disp(base) */
struct operand addr2(struct operand oper, int disp)
{
    struct operand o = oper;
    o.kind = OPR_ADDR;
    o.disp = disp;

    return o;
}

/* name(base) */
struct operand addr2_pc_rel(struct operand oper, const char *name, int label_id)
{
    struct operand o = oper;
    o.kind = OPR_ADDR;
    o.string = name;
    o.label_id = label_id;

    return o;
}

/* _main, .L001, ... */
struct operand str(const char *value)
{
    struct operand o = INIT_OPERAND;
    o.kind = OPR_STR;
    o.string = value;

    return o;
}

/* rdi, rsi, ... */
struct operand arg(int index)
{
    struct operand o = INIT_OPERAND;
    o.kind = OPR_REG;
    o.reg_table = ARG_REG__[index];

    return o;
}

/* .LBB1_2, ... */
struct operand label(int block_id, int label_id)
{
    struct operand o = {0};
    o.kind = OPR_LABEL;
    o.block_id = block_id;
    o.label_id = label_id;

    return o;
}

/* _LBB1_2, ... */
struct operand label__(const char *label_str, int label_id)
{
    struct operand o = {0};
    o.kind = OPR_LABEL__;
    o.string = label_str;
    o.label_id = label_id;

    return o;
}

static const char *reg(const struct operand *oper, int tag)
{
    if (oper->data_tag == VARI) {
        return oper->reg_table[tag];
    } else {
        return oper->reg_table[oper->data_tag];
    }
}

static int promote_tag(int tag, const struct operand *oper)
{
    int t;

    if (oper == NULL) {
        return tag;
    }

    /* register that holds address doesn't affect promotion
     * as we don't know the size of data the address points to
     */
    if (oper->kind == OPR_ADDR) {
        return tag;
    }

    t = oper->data_tag;
    return tag > t ? tag : t;
}

static void gen_opecode__(FILE *fp, int tag, const struct opecode *op, int *nchars)
{
    int len = 0;
    const char *sfx = "";

    if (att_syntax) {
        sfx = get_data_suffix(tag);
    }

    if (!op->has_suffix) {
        sfx = "";
    }

    fprintf(fp, "%s%s", op->mnemonic, sfx);

    len = strlen(op->mnemonic) + strlen(sfx);

    *nchars = len;
}

static void gen_pc_rel_addr(FILE *fp, const char *name, int label_id)
{
    if (label_id > 0)
        fprintf(fp, "_%s_%d", name, label_id);
    else
        fprintf(fp, "_%s", name);
}

static void gen_operand__(FILE *fp, int tag, const struct operand *oper)
{
    switch (oper->kind) {

    case OPR_NONE:
        break;

    case OPR_REG:
        if (att_syntax) {
            fprintf(fp, "%%%s", reg(oper, tag));
        } else {
            fprintf(fp, "%s", reg(oper, tag));
        }
        break;

    case OPR_ADDR:
        if (att_syntax) {
            if (oper->string) {
                gen_pc_rel_addr(fp, oper->string, oper->label_id);
                fprintf(fp, "(%%%s)", reg(oper, tag));
            } else
            if (oper->disp != 0) {
                fprintf(fp, "%d(%%%s)", oper->disp, reg(oper, tag));
            } else {
                fprintf(fp, "(%%%s)", reg(oper, tag));
            }
        } else {
            if (oper->disp != 0) {
                fprintf(fp, "%s [%s%+d]",
                    get_data_directive(tag), reg(oper, tag), oper->disp);
            } else {
                fprintf(fp, "%s [%s]", get_data_directive(tag), reg(oper, tag));
            }
        }
        break;

    case OPR_IMME:
        if (att_syntax) {
            fprintf(fp, "$%ld", oper->immediate);
        } else {
            fprintf(fp, "%ld", oper->immediate);
        }
        break;

    case OPR_LABEL:
        fprintf(fp, ".LBB%d_%d", oper->block_id, oper->label_id);
        break;

    case OPR_LABEL__:
        if (oper->label_id == 0)
            fprintf(fp, "_%s(%%rip)", oper->string);
        else
            fprintf(fp, "_%s.%d(%%rip)", oper->string, oper->label_id);
        break;

    case OPR_STR:
        fprintf(fp, "_%s", oper->string);
        break;

    default:
        break;
    }
}

static void code__(FILE *fp, int tag,
        const struct opecode *op, const struct operand *oper1, const struct operand *oper2)
{
    const struct operand *o1 = NULL, *o2 = NULL;
    int nchars = 0;
    int dtag_ = tag;

    dtag_ = promote_tag(dtag_, oper1);
    dtag_ = promote_tag(dtag_, oper2);

    fprintf(fp, "    ");

    gen_opecode__(fp, dtag_, op, &nchars);

    if (oper1 != NULL && oper2 == NULL) {
        o1 = oper1;
    } else {
        if (att_syntax) {
            o1 = oper1;
            o2 = oper2;
        } else {
            o1 = oper2;
            o2 = oper1;
        }
    }

    if (o1) {
        const int max_pad = 7;
        const int pad = nchars > max_pad ? 0 : max_pad - nchars;
        fprintf(fp, "%*s", pad, "");
        gen_operand__(fp, dtag_, o1);
    }

    if (o2) {
        fprintf(fp, ", ");
        gen_operand__(fp, dtag_, o2);
    }

    fprintf(fp, "\n");
}

static int data_tag_(const struct data_type *type)
{
    if (is_char(type))
        return BYTE;
    if (is_short(type))
        return WORD;
    if (is_int(type))
        return LONG;
    if (is_long(type))
        return QUAD;
    if (is_enum(type))
        return LONG;
    return QUAD;
#if 0
    const int size = get_size(type);

    switch (size) {
    case 1: return BYTE;
    case 2: return WORD;
    case 4: return LONG;
    case 8: return QUAD;
    default:
        /* TODO error handling */
        return QUAD;
    }
#endif
}

static int get_mem_offset(const struct ast_node *node)
{
    return node->sym->mem_offset;
}

static void code1__(FILE *fp, const struct ast_node *node,
        struct opecode op)
{
    const struct opecode o0 = op;
    const int tag = data_tag_(node->type);

    code__(fp, tag, &o0, NULL, NULL);
}

/* because of the return address is already pushed when a fuction starts
 * the rbp % 0x10 should be 0x08 */
static int stack_align = 8;

static void inc_stack_align(int n)
{
    stack_align += 8 * n;
}

static void dec_stack_align(int n)
{
    stack_align -= 8 * n;
}

static int need_adjust_stack_align(int stack_arg_count)
{
    return (stack_align + 8 * stack_arg_count) % 16 != 0;
}

static void code2__(FILE *fp, const struct ast_node *node,
        struct opecode op, struct operand oper1)
{
    const struct opecode o0 = op;
    const struct operand o1 = oper1;
    int tag = data_tag_(node->type);

    /* 64 bit mode supports only full register for pop and push */
    if (!strcmp(op.mnemonic, "push") ||
        !strcmp(op.mnemonic, "pop"))
        tag = QUAD;

    if (!strcmp(op.mnemonic, "push"))
        inc_stack_align(1);
    if (!strcmp(op.mnemonic, "pop"))
        dec_stack_align(1);

    code__(fp, tag, &o0, &o1, NULL);
}

static void code3__(FILE *fp, const struct ast_node *node,
        struct opecode op, struct operand oper1, struct operand oper2)
{
    struct opecode o0 = op;
    struct operand o1 = oper1;
    struct operand o2 = oper2;
    int tag = data_tag_(node->type);

    /* this rule comes from x86-64 machine instructions.
     * it depends on the size of register when loading from memory.
     * it is independent of language data types. */
    /* TODO move this to gen_load()? */
    if (!strcmp(op.mnemonic, "mov") &&
        oper1.kind == OPR_ADDR &&
        oper2.kind == OPR_REG)
    {
        switch (tag) {
        case BYTE:
            o0 = is_unsigned(node->type) ? MOVZB_ : MOVSB_;
            o2 = EAX;
            break;
        case WORD:
            o0 = is_unsigned(node->type) ? MOVZW_ : MOVSW_;
            o2 = EAX;
            break;
        default:
            break;
        }
    }

    code__(fp, tag, &o0, &o1, &o2);
}

/* forward declaration */
static void gen_code(FILE *fp, const struct ast_node *node);

static const struct ast_node *find_node(const struct ast_node *node, int node_kind)
{
    const struct ast_node *found = NULL;

    if (!node)
        return NULL;

    if (node->kind == node_kind)
        return node;

    found = find_node(node->l, node_kind);
    if (found)
        return found;

    found = find_node(node->r, node_kind);
    if (found)
        return found;

    return NULL;
}

static void gen_func_param_list_variadic_(FILE *fp)
{
    int i;
    for (i = 0; i < 6; i++) {
        struct ast_node dummy = {0};
        const int disp = -8 * (6 - i);
        code3__(fp, &dummy, MOV_, arg(i), addr2(RBP, disp));
    }
}

static void gen_func_param_list_(FILE *fp, const struct symbol *func_sym)
{
    const struct symbol *sym;
    int index = 0;

    for (sym = first_param(func_sym); sym; sym = next_param(sym)) {
        /* TODO consider removing dummy by changing node parameter to data_type */
        struct ast_node dummy = {0};
        int disp = 0;

        if (is_ellipsis(sym))
            break;

        dummy.type = sym->type;
        disp = -1 * sym->mem_offset;

        code3__(fp, &dummy, MOV_, arg(index), addr2(RBP, disp));
        index++;

        if (index == 6)
            break;
    }
}

static void gen_func_param_list(FILE *fp, const struct ast_node *node)
{
    const struct ast_node *fdecl, *func;

    fdecl = find_node(node, NOD_DECL_FUNC);
    func = find_node(fdecl->l, NOD_DECL_IDENT);

    if (is_variadic(func->sym))
        gen_func_param_list_variadic_(fp);
    else
        gen_func_param_list_(fp, func->sym);
}

static void gen_func_prologue(FILE *fp, const struct ast_node *node)
{
    const struct ast_node *ddecl = NULL;
    const struct ast_node *ident = NULL;

    /* TODO define find_node_last()? */
    ddecl = find_node(node, NOD_DECL_DIRECT);
    ident = find_node(ddecl->r, NOD_DECL_IDENT);
    /* TODO assert(ident) */

    if (!is_static(ident->sym))
        fprintf(fp, "    .global _%s\n", ident->sym->name);
    fprintf(fp, "_%s:\n", ident->sym->name);
    code2__(fp, ident, PUSH_, RBP);
    code3__(fp, ident, MOV_,  RSP, RBP);
    code3__(fp, ident, SUB_, imme(get_mem_offset(ident)), RSP);
}

static void gen_func_body(FILE *fp, const struct ast_node *node)
{
    const struct ast_node *body = NULL;

    if (0)
        body = find_node(node, NOD_COMPOUND);
        /* TODO assert(body) */
    body = node;

    gen_code(fp, body);
}

static void gen_func_epilogue(FILE *fp, const struct ast_node *node)
{
    code3__(fp, node, MOV_, RBP, RSP);
    code2__(fp, node, POP_, RBP);
    code1__(fp, node, RET_);
}

static void gen_func_call(FILE *fp, const struct ast_node *node)
{
    if (!node)
        return;

    switch (node->kind) {

    case NOD_CALL:
        {
            const struct symbol *func_sym = node->l->sym;
            const int arg_count = node->ival;
            const int stack_arg_count = arg_count > 6 ? arg_count - 6 : 0;
            const int adjust = need_adjust_stack_align(stack_arg_count);
            int i;

            /* Need to adjust stack alignment before pushing arguments
             * as we always want them to be the top of stack for functions
             * that take more than 6 arguments */
            /* adjust stack alignment */
            if (adjust)
                fprintf(fp, "    subq   $8, %%rsp ## alignment\n");

            /* push args to stack */
            gen_func_call(fp, node->r);
            /* pop args to registers (max number is 6) */
            for (i = 0; i < arg_count; i++) {
                if (i == 6)
                    break;
                code2__(fp, node, POP_, arg(i));
            }

            /* number of fp */
            if (is_variadic(func_sym))
                fprintf(fp, "    movl   $0, %%eax\n");
            /* TODO fix mov suffix with imme and eax) */
            /* code3__(fp, node, MOV_, imme(0), EAX); */

            /* call */
            code2__(fp, node, CALL_, str(func_sym->name));

            /* clean up arguments on stack */
            if (stack_arg_count > 0) {
                fprintf(fp, "    addq   $%d, %%rsp ## clean up arguments on stack\n",
                        8 * stack_arg_count);
                dec_stack_align(stack_arg_count);
            }

            /* adjust stack alignment */
            if (adjust)
                fprintf(fp, "    addq   $8, %%rsp ## alignment\n");

            return;
        }

    case NOD_ARG:
        /* push args */
        gen_code(fp, node->l);
        code2__(fp, node, PUSH_, RAX);
        return;

    default:
        /* walk tree from the rightmost arg */
        gen_func_call(fp, node->r);
        gen_func_call(fp, node->l);
        return;
    }
}

static void gen_builtin_va_start(FILE *fp)
{
    fprintf(fp, "## __builtin_va_start\n");
    /* pop arguments */
    fprintf(fp, "    pop    %%rax ## &va_list\n");
    fprintf(fp, "    pop    %%rdx ## &last\n");
    /* gp_offset */
    fprintf(fp, "    leaq   -48(%%rbp), %%rdi ## gp_offset\n");
    fprintf(fp, "    subq   %%rdi, %%rdx      ## gp_offset\n");
    fprintf(fp, "    addq   $8, %%rdx         ## gp_offset\n");
    fprintf(fp, "    movq   %%rdx, (%%rax)    ## gp_offset\n");
    /* fp_offset */
    fprintf(fp, "    movl   $48, 4(%%rax)     ## fp_offset\n");
    /* overflow_arg_area */
    fprintf(fp, "    leaq   16(%%rbp), %%rdi  ## overflow_arg_area\n");
    fprintf(fp, "    movq   %%rdi, 8(%%rax)   ## overflow_arg_area\n");
    /* reg_save_area */
    fprintf(fp, "    leaq   -48(%%rbp), %%rdi ## reg_save_area\n");
    fprintf(fp, "    movq   %%rdi, 16(%%rax)  ## reg_save_area\n");
    fprintf(fp, "## end of __builtin_va_start\n");
}

static void gen_func_call_builtin(FILE *fp, const struct ast_node *node)
{
    if (!node)
        return;

    switch (node->kind) {

    case NOD_CALL:
        {
            const struct symbol *func_sym = node->l->sym;

            /* push args to stack */
            gen_func_call_builtin(fp, node->r);

            /* call */
            if (!strcmp(func_sym->name, "__builtin_va_start"))
                gen_builtin_va_start(fp);

            return;
        }

    case NOD_ARG:
        /* push args */
        gen_code(fp, node->l);
        /* no count pushes and pops as builtins are not function calls */
        fprintf(fp, "    push   %%rax\n");
        return;

    default:
        /* walk tree from the rightmost arg */
        gen_func_call_builtin(fp, node->r);
        gen_func_call_builtin(fp, node->l);
        return;
    }
}

static void gen_comment(FILE *fp, const char *cmt)
{
    fprintf(fp, "## %s\n", cmt);
}

static void gen_label(FILE *fp, int block_id, int label_id)
{
    fprintf(fp, ".LBB%d_%d:\n", block_id, label_id);
}

static void gen_ident(FILE *fp, const struct ast_node *node)
{
    const struct symbol *sym;

    if (!node || !node->sym)
        return;

    sym = node->sym;

    if (is_global_var(sym)) {
        const int id = is_static(sym) ? sym->id : -1;
        if (is_array(node->type)) {
            code3__(fp, node, LEA_, addr2_pc_rel(RIP, sym->name, id), RAX);
        } else {
            code3__(fp, node, MOV_, addr2_pc_rel(RIP, sym->name, id), A_);
        }
    }
    else if (is_enumerator(sym)) {
        code3__(fp, node, MOV_, imme(get_mem_offset(node)), A_);
    }
    else {
        const int disp = -get_mem_offset(node);

        if (is_array(node->type)) {
            code3__(fp, node, LEA_, addr2(RBP, disp), A_);
        } else {
            code3__(fp, node, MOV_, addr2(RBP, disp), A_);
        }
    }
}

static void gen_ident_lvalue(FILE *fp, const struct ast_node *node)
{
    const struct symbol *sym;

    if (!node || !node->sym)
        return;

    sym = node->sym;

    if (is_global_var(sym)) {
        const int id = is_static(sym) ? sym->id : -1;
        code3__(fp, node, LEA_, addr2_pc_rel(RIP, sym->name, id), RAX);
    } else {
        code3__(fp, node, MOV_, BP_, RAX);
        code3__(fp, node, SUB_, imme(get_mem_offset(node)), RAX);
    }
}

static void gen_address(FILE *fp, const struct ast_node *node)
{
    if (node == NULL)
        return;

    switch (node->kind) {

        /* TODO need this for initialization. may not need this for IR */
    case NOD_DECL_IDENT:
    case NOD_IDENT:
        gen_ident_lvalue(fp, node);
        break;

    case NOD_DEREF:
        gen_code(fp, node->l);
        break;

    case NOD_STRUCT_REF:
        {
            const int disp = get_mem_offset(node->r);
            gen_address(fp, node->l);
            code3__(fp, node, ADD_, imme(disp), RAX);
        }
        break;

    default:
        gen_comment(fp, "not an lvalue");
        break;
    }
}

static void gen_load(FILE *fp, const struct ast_node *node)
{
    /* array objects cannot be loaded in registers, and converted to pointers */
    if (is_array(node->type))
        return;
    code3__(fp, node, MOV_, addr1(RAX), A_);
}

static void gen_store(FILE *fp, const struct ast_node *node, struct operand addr)
{
    if (is_long(node->type)) {
        if (is_unsigned(node->type))
            code3__(fp, node, MOV_, EAX, RAX);
        else
            code3__(fp, node, MOVSL_, EAX, RAX);
    }

    code3__(fp, node, MOV_, A_, addr1(addr));
}

static void gen_div(FILE *fp, const struct ast_node *node, struct operand divider)
{
    /* rax -> rdx:rax (zero extend) */
    if (is_unsigned(node->type)) {
        code3__(fp, node, XOR_, D_, D_);
        code2__(fp, node, DIV_, divider);
        return;
    }

    /* rax -> rdx:rax (signed extend) */
    if (is_long(node->type))
        code1__(fp, node, CQTO_);
    else
        code1__(fp, node, CLTD_);

    code2__(fp, node, IDIV_, divider);
}

static void gen_preincdec(FILE *fp, const struct ast_node *node, struct opecode op)
{
    int sz = 1;
    if (is_pointer(node->type))
        sz = get_size(underlying(node->type));

    gen_address(fp, node->l);
    code3__(fp, node, op, imme(sz), addr1(RAX));
    code3__(fp, node, MOV_, addr1(RAX), A_);
}

static void gen_postincdec(FILE *fp, const struct ast_node *node, struct opecode op)
{
    int sz = 1;
    if (is_pointer(node->type))
        sz = get_size(underlying(node->type));

    gen_address(fp, node->l);
    code3__(fp, node, MOV_, RAX, RDX);
    code3__(fp, node, MOV_, addr1(RAX), A_);
    code3__(fp, node, op, imme(sz), addr1(RDX));
}

static void gen_relational(FILE *fp, const struct ast_node *node, struct opecode op)
{
    gen_code(fp, node->l);
    code2__(fp, node, PUSH_, RAX);
    gen_code(fp, node->r);
    code3__(fp, node, MOV_, A_, D_);
    code2__(fp, node, POP_, RAX);
    code3__(fp, node, CMP_, D_, A_);
    code2__(fp, node, op, AL);
    code3__(fp, node, MOVZB_, AL, A_);
}

static void gen_equality(FILE *fp, const struct ast_node *node, struct opecode op)
{
    gen_code(fp, node->l);
    code2__(fp, node, PUSH_, RAX);
    gen_code(fp, node->r);
    code2__(fp, node, POP_, RDX);
    code3__(fp, node, CMP_, D_, A_);
    code2__(fp, node, op,   AL);
    code3__(fp, node, MOVZB_, AL, A_);
}

enum jump_kind {
    JMP_RETURN = 0,
    JMP_ENTER,
    JMP_EXIT,
    JMP_ELSE,
    JMP_CONTINUE,
    JMP_OFFSET = 100
};

struct jump_scope {
    int curr;
    int brk;
    int conti;
    int func;
};

static int jump_id(const struct ast_node *node)
{
    return JMP_OFFSET + node->sym->id;
}

static void gen_switch_table_(FILE *fp, const struct ast_node *node, int switch_scope)
{
    if (!node)
        return;

    switch (node->kind) {

    case NOD_SWITCH:
        /* skip nested switch */
        return;

    case NOD_CASE:
        code3__(fp, node, CMP_, imme(node->l->ival), A_);
        code2__(fp, node, JE_,  label(switch_scope, jump_id(node)));
        /* check next statement if it is another case statement */
        gen_switch_table_(fp, node->r, switch_scope);
        return;

    case NOD_DEFAULT:
        code2__(fp, node, JMP_,  label(switch_scope, jump_id(node)));
        return;

    case NOD_COMPOUND:
    case NOD_LIST:
        gen_switch_table_(fp, node->l, switch_scope);
        gen_switch_table_(fp, node->r, switch_scope);
        break;

    default:
        break;
    }
}

static void gen_switch_table(FILE *fp, const struct ast_node *node, int switch_scope)
{
    gen_comment(fp, "begin jump table");
    gen_switch_table_(fp, node, switch_scope);
    /* for switch without default */
    code2__(fp, node, JMP_, label(switch_scope, JMP_EXIT));
    gen_comment(fp, "end jump table");
}

static void gen_cast(FILE *fp, const struct ast_node *node)
{
    struct data_type *to;

    if (is_pointer(node->type))
        return;

    to = node->type;

    if (is_char(to)) {
        if (is_unsigned(to))
            code3__(fp, node, MOVZB_, AL, EAX);
        else
            code3__(fp, node, MOVSB_, AL, EAX);
    }
    else if (is_short(to)) {
        if (is_unsigned(to))
            code3__(fp, node, MOVZW_, AX, EAX);
        else
            code3__(fp, node, MOVSW_, AX, EAX);
    }
    else if (is_long(to)) {
        if (is_unsigned(to))
            code3__(fp, node, MOV_,   EAX, RAX);
        else
            code3__(fp, node, MOVSL_, EAX, RAX);
    }
}

static void gen_init_scalar_local(FILE *fp, const struct data_type *type,
        const struct ast_node *ident, int offset, const struct ast_node *expr)
{
    /* TODO fix */
    struct ast_node dummy = {0};
    dummy.type = (struct data_type *) type;

    gen_comment(fp, "local scalar init");

    /* ident */
    gen_address(fp, ident);
    if (offset > 0)
        code3__(fp, ident, ADD_, imme(offset), A_);
    code2__(fp, ident, PUSH_, RAX);

    if (expr) {
        /* init expr */
        gen_code(fp, expr);
        /* assign expr */
        code2__(fp, &dummy, POP_,  RDX);
        code3__(fp, &dummy, MOV_, A_, addr1(RDX));
    } else {
        /* assign zero */
        code2__(fp, &dummy, POP_,  RDX);
        code3__(fp, &dummy, MOV_, imme(0), addr1(RAX));
    }
}

struct memory_byte {
    const struct ast_node *init;
    const struct data_type *type;
    int is_written;
    int written_size;
};

struct object_byte {
    struct memory_byte *bytes;
    struct symbol *sym;
    int size;
};

static void print_object(struct object_byte *obj)
{
    int i;

    for (i = 0; i < obj->size; i++) {
        printf("    [%04d] is_written: %d written_size: %d init: %p\n",
                i,
                obj->bytes[i].is_written,
                obj->bytes[i].written_size,
                (void *) obj->bytes[i].init);
    }
}

static void zero_clear_bytes(struct memory_byte *bytes, const struct data_type *type)
{
    if (is_array(type)) {
        const int len = get_array_length(type);
        const int elem_size = get_size(underlying(type));
        int i;

        for (i = 0; i < len; i++) {
            const base = elem_size * i;

            zero_clear_bytes(bytes + base, underlying(type));
        }
    }
    else if (is_struct(type)) {
        const struct symbol *sym;

        for (sym = first_member(type->sym); sym; sym = next_member(sym))
            zero_clear_bytes(bytes + sym->mem_offset, sym->type);
    }
    else {
        /* scalar */
        const int size = get_size(type);
        int i;
        for (i = 0; i < size; i++) {
            bytes[i].is_written = 1;
            bytes[i].written_size = (i == 0) ? size : 0;
            bytes[i].type = type;
        }
    }
}

static void init_object_byte(struct object_byte *obj, const struct ast_node *ident)
{
    const size = get_size(ident->type);

    obj->bytes = (struct memory_byte *) calloc(size, sizeof(struct memory_byte));
    obj->size = size;
    obj->sym = ident->sym;

    zero_clear_bytes(obj->bytes, obj->sym->type);
}

static void free_object_byte(struct object_byte *obj)
{
    struct object_byte o = {0};
    free(obj->bytes);
    *obj = o;
}

static void assign_init(struct memory_byte *base,
        const struct data_type *type, const struct ast_node *node)
{
    if (!node)
        return;

    switch (node->kind) {

    case NOD_INIT:
        {
            /* move cursor by designator */
            const int offset = node->ival;

            assign_init(base, type, node->l);
            assign_init(base + offset, underlying(type), node->r);
        }
        break;

    case NOD_LIST:
        /* pass to the next initializer */
        assign_init(base, type, node->l);
        assign_init(base, type, node->r);
        break;

    default:
        {
            /* assign initializer to byte */
            const int size = base->written_size;
            int i;

            base->init = node;

            for (i = 0; i < size; i++)
                base[i].is_written = 1;
        }
        break;
    }
}

static void gen_init_scalar_global(FILE *fp, const struct data_type *type,
        const struct ast_node *expr)
{
    const int tag = data_tag_(type);
    const char *szname = get_data_name(tag);

    if (!expr) {
        fprintf(fp, "    .%s 0\n", szname);
        return;
    }

    switch (expr->kind) {

    case NOD_NUM:
        fprintf(fp, "    .%s %ld\n", szname, expr->ival);
        break;

    case NOD_ADDR:
        {
            /* TODO make function taking sym */
            const struct symbol *sym = expr->l->sym;
            fprintf(fp, "    .%s ", szname);
            if (is_static(sym))
                gen_pc_rel_addr(fp, sym->name, sym->id);
            else
                gen_pc_rel_addr(fp, sym->name, -1);
            fprintf(fp, "\n");
        }
        break;

    default:
        break;
    }
}

static void gen_object_byte(FILE *fp, const struct object_byte *obj)
{
    int i;

    {
        struct symbol *sym = obj->sym;
        int id = sym->id;

        /* TODO make function taking sym */
        if (!is_static(sym)) {
            id = -1;
            fprintf(fp, "    .global ");
            gen_pc_rel_addr(fp, sym->name, id);
            fprintf(fp, "\n");
        }
        gen_pc_rel_addr(fp, sym->name, id);
        fprintf(fp, ":\n");
    }

    for (i = 0; i < obj->size; i++) {
        const struct memory_byte *byte = &obj->bytes[i];

        if (!byte->is_written) {
            /* TODO count skipping bytes */
            fprintf(fp, "    .zero 1\n");
            continue;
        }

        if (byte->written_size > 0)
            gen_init_scalar_global(fp, byte->type, byte->init);
    }

    fprintf(fp, "\n");
}

static void gen_initializer(FILE *fp,
        const struct ast_node *ident, const struct ast_node *init)
{
    struct object_byte obj = {0};

    init_object_byte(&obj, ident);
    assign_init(obj.bytes, ident->type, init);

    if (0)
        print_object(&obj);

    if (is_global_var(ident->sym) && !is_extern(ident->sym)) {
        gen_object_byte(fp, &obj);
    }
    else if (is_local_var(ident->sym)) {
        int i;

        for (i = 0; i < obj.size; i++) {
            const struct memory_byte *byte = &obj.bytes[i];

            if (byte->written_size > 0)
                gen_init_scalar_local(fp, byte->type, ident, i, byte->init);
        }
    }

    free_object_byte(&obj);
}

static void gen_initializer_global(FILE *fp, const struct ast_node *node)
{
    const struct ast_node *ident;

    if (!node || node->kind != NOD_DECL_INIT)
        return;

    ident = find_node(node->l, NOD_DECL_IDENT);

    if (ident && is_global_var(ident->sym))
        gen_initializer(fp, ident, node->r);
}

static void gen_initializer_local(FILE *fp, const struct ast_node *node)
{
    const struct ast_node *ident;

    if (!node || !node->r || node->kind != NOD_DECL_INIT)
        return;

    ident = find_node(node->l, NOD_DECL_IDENT);

    if (ident && is_local_var(ident->sym))
        gen_initializer(fp, ident, node->r);
}

static void gen_code(FILE *fp, const struct ast_node *node)
{
    static struct jump_scope scope = {0};
    struct jump_scope tmp = {0};
    static int next_scope = 0;

    if (node == NULL)
        return;

    switch (node->kind) {

    case NOD_LIST:
    case NOD_COMPOUND:
    case NOD_DECL:
        gen_code(fp, node->l);
        gen_code(fp, node->r);
        break;

    case NOD_FOR:
        tmp = scope;
        scope.curr = next_scope++;
        scope.brk = scope.curr;
        scope.conti = scope.curr;

        gen_code(fp, node->l);
        gen_code(fp, node->r);

        scope = tmp;
        break;

    case NOD_FOR_PRE_COND:
        /* pre */
        gen_comment(fp, "for-pre");
        gen_code(fp, node->l);
        /* cond */
        gen_comment(fp, "for-cond");
        gen_label(fp, scope.curr, JMP_ENTER);
        gen_code(fp, node->r);
        code3__(fp, node, CMP_, imme(0), A_);
        code2__(fp, node, JE_,  label(scope.curr, JMP_EXIT));
        break;

    case NOD_FOR_BODY_POST:
        /* body */
        gen_comment(fp, "for-body");
        gen_code(fp, node->l);
        /* post */
        gen_comment(fp, "for-post");
        gen_label(fp, scope.curr, JMP_CONTINUE);
        gen_code(fp, node->r);
        code2__(fp, node, JMP_, label(scope.curr, JMP_ENTER));
        gen_label(fp, scope.curr, JMP_EXIT);
        break;

    case NOD_WHILE:
        tmp = scope;
        scope.curr = next_scope++;
        scope.brk = scope.curr;
        scope.conti = scope.curr;

        gen_comment(fp, "while-cond");
        gen_label(fp, scope.curr, JMP_CONTINUE);
        gen_code(fp, node->l);
        code3__(fp, node, CMP_, imme(0), A_);
        code2__(fp, node, JE_,  label(scope.curr, JMP_EXIT));
        gen_comment(fp, "while-body");
        gen_code(fp, node->r);
        code2__(fp, node, JMP_, label(scope.curr, JMP_CONTINUE));
        gen_label(fp, scope.curr, JMP_EXIT);

        scope = tmp;
        break;

    case NOD_DOWHILE:
        tmp = scope;
        scope.curr = next_scope++;
        scope.brk = scope.curr;
        scope.conti = scope.curr;

        gen_comment(fp, "do-while-body");
        gen_label(fp, scope.curr, JMP_ENTER);
        gen_code(fp, node->l);
        gen_comment(fp, "do-while-cond");
        gen_label(fp, scope.curr, JMP_CONTINUE);
        gen_code(fp, node->r);
        code3__(fp, node, CMP_, imme(0), A_);
        code2__(fp, node, JE_,  label(scope.curr, JMP_EXIT));
        code2__(fp, node, JMP_, label(scope.curr, JMP_ENTER));
        gen_label(fp, scope.curr, JMP_EXIT);

        scope = tmp;
        break;

    case NOD_IF:
        /* if */
        tmp = scope;
        scope.curr = next_scope++;

        gen_comment(fp, "if-cond");
        gen_code(fp, node->l);
        code3__(fp, node, CMP_, imme(0), A_);
        code2__(fp, node, JE_,  label(scope.curr, JMP_ELSE));
        gen_code(fp, node->r);

        scope = tmp;
        break;

    case NOD_IF_THEN:
        /* then */
        gen_comment(fp, "if-then");
        gen_code(fp, node->l);
        code2__(fp, node, JMP_, label(scope.curr, JMP_EXIT));
        /* else */
        gen_comment(fp, "if-else");
        gen_label(fp, scope.curr, JMP_ELSE);
        gen_code(fp, node->r);
        gen_label(fp, scope.curr, JMP_EXIT);
        break;

    case NOD_SWITCH:
        tmp = scope;
        scope.curr = next_scope++;
        scope.brk = scope.curr;

        gen_comment(fp, "switch-value");
        gen_code(fp, node->l);
        gen_switch_table(fp, node->r, scope.curr);
        gen_code(fp, node->r);
        gen_label(fp, scope.curr, JMP_EXIT);

        scope = tmp;
        break;

    case NOD_CASE:
    case NOD_DEFAULT:
        gen_label(fp, scope.curr, jump_id(node));
        gen_code(fp, node->r);
        break;

    case NOD_RETURN:
        gen_code(fp, node->l);
        code2__(fp, node, JMP_, label(scope.func, JMP_RETURN));
        break;

    case NOD_BREAK:
        code2__(fp, node, JMP_, label(scope.brk, JMP_EXIT));
        break;

    case NOD_CONTINUE:
        code2__(fp, node, JMP_, label(scope.conti, JMP_CONTINUE));
        break;

    case NOD_LABEL:
        gen_label(fp, scope.func, jump_id(node->l));
        gen_code(fp, node->r);
        break;

    case NOD_GOTO:
        code2__(fp, node, JMP_, label(scope.func, jump_id(node->l)));
        break;

    case NOD_IDENT:
        gen_ident(fp, node);
        break;

    case NOD_DECL_INIT:
        gen_initializer_local(fp, node);
        break;

    case NOD_STRUCT_REF:
        gen_address(fp, node);
        gen_load(fp, node);
        break;

    case NOD_CALL:
        if (is_builtin(node->l->sym))
            gen_func_call_builtin(fp, node);
        else
            gen_func_call(fp, node);
        break;

    case NOD_FUNC_DEF:
        tmp = scope;
        scope.curr = next_scope++;
        scope.func = scope.curr;

        gen_func_prologue(fp, node->l);
        gen_comment(fp, "func params");
        gen_func_param_list(fp, node->l);
        gen_comment(fp, "func body");
        gen_func_body(fp, node->r);
        gen_label(fp, scope.curr, JMP_RETURN);
        gen_func_epilogue(fp, node->r);

        scope = tmp;
        break;

    case NOD_ASSIGN:
        gen_comment(fp, "assign");
        gen_address(fp, node->l);
        code2__(fp, node, PUSH_, RAX);
        gen_code(fp, node->r);
        code2__(fp, node, POP_,  RDX);

        /* TODO consider adding cast explicitly */
        if (0)
            gen_store(fp, node, RDX);

        if (is_long(node->type) && !is_long(node->r->type)) {
            if (is_unsigned(node->type))
                code3__(fp, node, MOV_, EAX, RAX);
            else
                code3__(fp, node, MOVSL_, EAX, RAX);
        }

        code3__(fp, node, MOV_, A_, addr1(RDX));
        break;

    case NOD_ADD_ASSIGN:
        gen_comment(fp, "add-assign");
        gen_address(fp, node->l);
        code2__(fp, node, PUSH_, RAX);
        gen_code(fp, node->r);
        code2__(fp, node, POP_,  RDX);
        code3__(fp, node, ADD_, A_, addr1(RDX));
        code3__(fp, node, MOV_, addr1(RDX), A_);
        break;

    case NOD_SUB_ASSIGN:
        gen_comment(fp, "sub-assign");
        gen_address(fp, node->l);
        code2__(fp, node, PUSH_, RAX);
        gen_code(fp, node->r);
        code2__(fp, node, POP_,  RDX);
        code3__(fp, node, SUB_, A_, addr1(RDX));
        code3__(fp, node, MOV_, addr1(RDX), A_);
        break;

    case NOD_MUL_ASSIGN:
        gen_comment(fp, "mul-assign");
        gen_address(fp, node->l);
        code2__(fp, node, PUSH_, RAX);
        gen_code(fp, node->r);
        code2__(fp, node, POP_,  RDX);
        code3__(fp, node, IMUL_, addr1(RDX), A_);
        code3__(fp, node, MOV_, A_, addr1(RDX));
        break;

    case NOD_DIV_ASSIGN:
        gen_comment(fp, "div-assign");
        gen_address(fp, node->l);
        code2__(fp, node, PUSH_, RAX);
        gen_code(fp, node->r);
        code3__(fp, node, MOV_, A_, DI_);
        code2__(fp, node, POP_, RSI);
        code3__(fp, node, MOV_, addr1(RSI), A_);
        code1__(fp, node, CLTD_); /* rax -> rdx:rax */
        code2__(fp, node, IDIV_, DI_);
        code3__(fp, node, MOV_, A_, addr1(RSI));
        break;

    case NOD_ADDR:
        gen_address(fp, node->l);
        break;

    case NOD_CAST:
        gen_code(fp, node->r);
        gen_cast(fp, node);
        break;

    case NOD_DEREF:
        gen_code(fp, node->l);
        gen_load(fp, node);
        break;

    case NOD_NUM:
        code3__(fp, node, MOV_, imme(node->ival), A_);
        break;

    case NOD_STRING:
        code3__(fp, node, LEA_, label__("L.str", node->sym->id), A_);
        break;

    case NOD_SIZEOF:
        code3__(fp, node, MOV_, imme(get_size(node->l->type)), A_);
        break;

    case NOD_ADD:
        gen_code(fp, node->l);
        code2__(fp, node, PUSH_, RAX);
        gen_code(fp, node->r);
        code2__(fp, node, POP_, RDX);

        /* TODO find the best place to handle array subscript */
        if (is_array(node->l->type) || is_pointer(node->l->type)) {
            const int sz = get_size(underlying(node->l->type));
            code3__(fp, node, IMUL_, imme(sz), RAX);
        }

        code3__(fp, node, ADD_, D_, A_);
        break;

    case NOD_SUB:
        gen_code(fp, node->l);
        code2__(fp, node, PUSH_, RAX);
        gen_code(fp, node->r);
        code3__(fp, node, MOV_, A_, D_);
        code2__(fp, node, POP_, RAX);
        code3__(fp, node, SUB_, D_, A_);
        break;

    case NOD_MUL:
        gen_code(fp, node->l);
        code2__(fp, node, PUSH_, RAX);
        gen_code(fp, node->r);
        code2__(fp, node, POP_,  RDX);
        code3__(fp, node, IMUL_, D_, A_);
        break;

    case NOD_DIV:
        gen_code(fp, node->l);
        code2__(fp, node, PUSH_, RAX);
        gen_code(fp, node->r);
        code3__(fp, node, MOV_, A_, DI_);
        code2__(fp, node, POP_, RAX);
        code1__(fp, node, CLTD_); /* rax -> rdx:rax */
        code2__(fp, node, IDIV_, DI_);
        break;

    case NOD_MOD:
        gen_code(fp, node->l);
        code2__(fp, node, PUSH_, RAX);
        gen_code(fp, node->r);
        code3__(fp, node, MOV_, A_, DI_);
        code2__(fp, node, POP_, RAX);
        gen_div(fp, node, DI_);
        code3__(fp, node, MOV_, D_, A_);
        break;

    case NOD_NOT:
        gen_code(fp, node->l);
        code3__(fp, node, CMP_, imme(0), A_);
        code2__(fp, node, SETE_,  AL);
        code3__(fp, node, MOVZB_, AL, A_);
        break;

    case NOD_COND:
        /* cond */
        gen_comment(fp, "cond-?");
        tmp = scope;
        scope.curr = next_scope++;

        gen_code(fp, node->l);
        code3__(fp, node, CMP_, imme(0), A_);
        code2__(fp, node, JE_,  label(scope.curr, JMP_ELSE));
        gen_code(fp, node->r);

        scope = tmp;
        break;

    case NOD_COND_THEN:
        /* then */
        gen_comment(fp, "cond-then");
        gen_code(fp, node->l);
        code2__(fp, node, JMP_, label(scope.curr, JMP_EXIT));
        /* else */
        gen_comment(fp, "cond-else");
        gen_label(fp, scope.curr, JMP_ELSE);
        gen_code(fp, node->r);
        gen_label(fp, scope.curr, JMP_EXIT);
        break;

    case NOD_LOGICAL_OR:
        tmp = scope;
        scope.curr = next_scope++;

        gen_code(fp, node->l);
        code3__(fp, node, CMP_, imme(0), A_);
        code2__(fp, node, JNE_, label(scope.curr, JMP_CONTINUE));
        gen_code(fp, node->r);
        code3__(fp, node, CMP_, imme(0), A_);
        code2__(fp, node, JE_,  label(scope.curr, JMP_EXIT));
        gen_label(fp, scope.curr, JMP_CONTINUE);
        code3__(fp, node, MOV_, imme(1), A_);
        gen_label(fp, scope.curr, JMP_EXIT);

        scope = tmp;
        break;

    case NOD_LOGICAL_AND:
        tmp = scope;
        scope.curr = next_scope++;

        gen_code(fp, node->l);
        code3__(fp, node, CMP_, imme(0), A_);
        code2__(fp, node, JE_,  label(scope.curr, JMP_EXIT));
        gen_code(fp, node->r);
        code3__(fp, node, CMP_, imme(0), A_);
        code2__(fp, node, JE_,  label(scope.curr, JMP_EXIT));
        code3__(fp, node, MOV_, imme(1), A_);
        gen_label(fp, scope.curr, JMP_EXIT);

        scope = tmp;
        break;

    case NOD_PREINC:
        gen_preincdec(fp, node, ADD_);
        break;

    case NOD_PREDEC:
        gen_preincdec(fp, node, SUB_);
        break;

    case NOD_POSTINC:
        gen_postincdec(fp, node, ADD_);
        break;

    case NOD_POSTDEC:
        gen_postincdec(fp, node, SUB_);
        break;

    case NOD_LT:
        gen_relational(fp, node, SETL_);
        break;

    case NOD_GT:
        gen_relational(fp, node, SETG_);
        break;

    case NOD_LE:
        gen_relational(fp, node, SETLE_);
        break;

    case NOD_GE:
        gen_relational(fp, node, SETGE_);
        break;

    case NOD_EQ:
        gen_equality(fp, node, SETE_);
        break;

    case NOD_NE:
        gen_equality(fp, node, SETNE_);
        break;

    default:
        break;
    }
}

static void gen_global_vars(FILE *fp, const struct ast_node *node)
{
    if (!node)
        return;

    if (node->kind == NOD_DECL_INIT) {
        gen_initializer_global(fp, node);
        return;
    }

    gen_global_vars(fp, node->l);
    gen_global_vars(fp, node->r);
}

static void gen_string_literal(FILE *fp, const struct symbol_table *table)
{
    struct symbol *sym;

    for (sym = table->head; sym; sym = sym->next) {
        if (is_string_literal(sym)) {
            const char *p;
            fprintf(fp, "_L.str.%d:\n", sym->id);
            fprintf(fp, "    .asciz \"");
            for (p = sym->name; *p; p++) {
                switch (*p) {
                case '\0': fprintf(fp, "\\0"); break;
                case '\\': fprintf(fp, "\\");  break;
                case '\a': fprintf(fp, "\\a"); break;
                case '\b': fprintf(fp, "\\b"); break;
                case '\f': fprintf(fp, "\\f"); break;
                case '\n': fprintf(fp, "\\n"); break;
                case '\r': fprintf(fp, "\\r"); break;
                case '\t': fprintf(fp, "\\t"); break;
                case '\v': fprintf(fp, "\\v"); break;
                default:   fprintf(fp, "%c", *p); break;
                }
            }
            fprintf(fp, "\"\n\n");
        }
    }
}

void gen_x86(FILE *fp,
        const struct ast_node *tree, const struct symbol_table *table)
{
    if (!att_syntax) {
        fprintf(fp, ".intel_syntax noprefix\n");
    }

    fprintf(fp, "    .data\n\n");
    gen_string_literal(fp, table);
    gen_global_vars(fp, tree);

    fprintf(fp, "    .text\n\n");
    gen_code(fp, tree);
}

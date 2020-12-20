#ifndef PARSE_H
#define PARSE_H

#include "ast.h"
#include "lexer.h"
#include "symbol.h"
#include "message.h"
#include "type.h"

#define TOKEN_BUFFER_SIZE 2

struct parser {
    struct lexer lex;
    struct token tokbuf[TOKEN_BUFFER_SIZE];
    int head, curr;

    struct symbol_table symtbl;
    struct message_list *msg;

    long error_pos;
    const char *error_msg;

    /* ADDSYM */
    int decl_kind;
    const char * decl_ident;
    struct data_type *decl_type;
};

extern struct parser *new_parser();
extern void free_parser(struct parser *p);

extern void parser_init(struct parser *p);

extern struct ast_node *parse(struct parser *p);

#endif /* _H */

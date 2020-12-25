#ifndef MESSAGE_H
#define MESSAGE_H

#include "lexer.h"
/* TODO for struct position. remove this include */

#define MAX_MESSAGE_COUNT 5

struct message {
    const char *str;
    long file_pos;
    struct position pos;
};

struct message_list {
    struct message warnings[MAX_MESSAGE_COUNT];
    struct message errors[MAX_MESSAGE_COUNT];

    int warning_count;
    int error_count;
};

extern struct message_list *new_message_list();
extern void free_message_list(struct message_list *list);

extern void add_warning(struct message_list *list,
        const char *msg, const struct position *pos);
extern void add_error(struct message_list *list,
        const char *msg, const struct position *pos);

extern void print_warning_messages(FILE *fp, const char *filename,
        const struct message_list *list);
extern void print_error_messages(FILE *fp, const char *filename,
        const struct message_list *list);

#endif /* MESSAGE_H */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    NODE_NUMBER, NODE_VAR, NODE_BINARY, NODE_LET, NODE_PRINT,
    NODE_WHILE, NODE_IF, NODE_BLOCK, NODE_CALL, NODE_RETURN
} NodeType;

typedef struct Node {
    NodeType type;
    double num_val;
    char *name;
    int op;
    struct Node *left;
    struct Node *right;
    struct Node *body;
    struct Node *else_body;
    struct Node *args;
    struct Node *next;
} Node;

Node *parse_program(void);

#endif

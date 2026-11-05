#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static Token current;
static void advance(void) { current = lexer_next(); }

static Node *new_node(NodeType type) {
    Node *n = calloc(1, sizeof(Node)); n->type = type; return n;
}

static Node *parse_expression(void);

static Node *parse_primary(void) {
    if (current.type == TOK_NUMBER) {
        Node *n = new_node(NODE_NUMBER); n->num_val = current.num_val; advance(); return n;
    }
    if (current.type == TOK_MINUS) {
        advance();
        Node *n = new_node(NODE_BINARY); n->op = TOK_MINUS;
        n->left = new_node(NODE_NUMBER); n->left->num_val = 0.0;
        n->right = parse_primary(); return n;
    }
    if (current.type == TOK_IDENT) {
        Node *n = new_node(NODE_VAR); n->name = strdup(current.str_val); advance();
        if (current.type == TOK_LPAREN) {
            Node *call = new_node(NODE_CALL); call->name = n->name; advance();
            while (current.type != TOK_RPAREN && current.type != TOK_EOF) {
                Node *arg = parse_expression();
                if (!call->args) call->args = arg;
                else { Node *t = call->args; while (t->next) t = t->next; t->next = arg; }
                if (current.type != TOK_RPAREN) advance();
            }
            advance(); return call;
        }
        return n;
    }
    if (current.type == TOK_LPAREN) {
        advance(); Node *n = parse_expression();
        if (current.type == TOK_RPAREN) advance();
        return n;
    }
    return NULL;
}

static Node *parse_term(void) {
    Node *left = parse_primary();
    while (current.type == TOK_STAR || current.type == TOK_SLASH || current.type == TOK_PERCENT) {
        Node *n = new_node(NODE_BINARY); n->op = current.type; n->left = left;
        advance(); n->right = parse_primary(); left = n;
    }
    return left;
}

static Node *parse_expression(void) {
    Node *left = parse_term();
    while (current.type == TOK_PLUS || current.type == TOK_MINUS ||
           current.type == TOK_LT || current.type == TOK_GT ||
           current.type == TOK_LE || current.type == TOK_GE) {
        Node *n = new_node(NODE_BINARY); n->op = current.type; n->left = left;
        advance(); n->right = parse_term(); left = n;
    }
    return left;
}

static Node *parse_block(int consume_rbrace);

static Node *parse_statement(void) {
    Node *stmt = NULL;
    if (current.type == TOK_LET) {
        advance(); stmt = new_node(NODE_LET); stmt->name = strdup(current.str_val);
        advance(); if (current.type == TOK_ASSIGN) advance();
        stmt->left = parse_expression();
    } else if (current.type == TOK_PRINT) {
        advance(); stmt = new_node(NODE_PRINT); stmt->left = parse_expression();
    } else if (current.type == TOK_WHILE) {
        advance(); stmt = new_node(NODE_WHILE); stmt->left = parse_expression();
        if (current.type == TOK_COLON) advance();
        if (current.type == TOK_LBRACE) advance();
        stmt->body = parse_block(1);
    } else if (current.type == TOK_IF) {
        advance(); stmt = new_node(NODE_IF); stmt->left = parse_expression();
        if (current.type == TOK_COLON) advance();
        if (current.type == TOK_LBRACE) advance();
        stmt->body = parse_block(1);
        if (current.type == TOK_ELSE) {
            advance();
            if (current.type == TOK_COLON) advance();
            if (current.type == TOK_LBRACE) advance();
            stmt->else_body = parse_block(1);
        }
    } else {
        stmt = parse_expression();
    }
    return stmt;
}

static Node *parse_block(int consume_rbrace) {
    Node *block = new_node(NODE_BLOCK);
    Node *tail = NULL;
    while (current.type != TOK_EOF && current.type != TOK_RBRACE) {
        if (current.type == TOK_NEWLINE) { advance(); continue; }
        Node *stmt = parse_statement();
        if (!stmt) { advance(); continue; }
        if (tail) tail->next = stmt; else block->body = stmt;
        tail = stmt;
        if (current.type == TOK_NEWLINE) advance();
    }
    if (consume_rbrace && current.type == TOK_RBRACE) advance();
    return block;
}

Node *parse_program(void) {
    advance();
    return parse_block(0);
}

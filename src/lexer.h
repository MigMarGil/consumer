#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOK_EOF,
    TOK_NUMBER,
    TOK_IDENT,
    TOK_LET,
    TOK_PRINT,
    TOK_IF,
    TOK_ELSE,
    TOK_WHILE,
    TOK_FN,
    TOK_RETURN,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_COLON,
    TOK_ASSIGN,
    TOK_LT,
    TOK_GT,
    TOK_NEWLINE
} TokenType;

typedef struct {
    TokenType type;
    double num_val;
    char *str_val;
} Token;

void lexer_init(const char *source);
Token lexer_next(void);

#endif

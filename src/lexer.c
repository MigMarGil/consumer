#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char *src;
static int pos;

static Token make_token(TokenType type) {
    Token t; t.type = type; t.num_val = 0; t.str_val = NULL; return t;
}

void lexer_init(const char *source) { src = source; pos = 0; }

Token lexer_next(void) {
    while (src[pos] != '\0') {
        char c = src[pos];
        if (c == ' ' || c == '\t' || c == '\r') { pos++; continue; }
        if (c == '\n') { pos++; return make_token(TOK_NEWLINE); }
        if (c == '#') { while (src[pos] && src[pos] != '\n') pos++; continue; }
        if (isdigit(c) || (c == '.' && isdigit(src[pos+1]))) {
            char *end; double val = strtod(src + pos, &end); pos = end - src;
            Token t = make_token(TOK_NUMBER); t.num_val = val; return t;
        }
        if (isalpha(c) || c == '_') {
            int start = pos;
            while (isalnum(src[pos]) || src[pos] == '_') pos++;
            int len = pos - start;
            char *word = malloc(len + 1);
            strncpy(word, src + start, len); word[len] = '\0';
            Token t = make_token(TOK_IDENT);
            if (!strcmp(word, "let")) t.type = TOK_LET;
            else if (!strcmp(word, "print")) t.type = TOK_PRINT;
            else if (!strcmp(word, "if")) t.type = TOK_IF;
            else if (!strcmp(word, "else")) t.type = TOK_ELSE;
            else if (!strcmp(word, "while")) t.type = TOK_WHILE;
            else t.str_val = word;
            return t;
        }
        pos++;
        switch (c) {
            case '+': return make_token(TOK_PLUS);
            case '-': return make_token(TOK_MINUS);
            case '*': return make_token(TOK_STAR);
            case '/': return make_token(TOK_SLASH);
            case '%': return make_token(TOK_PERCENT);
            case '(': return make_token(TOK_LPAREN);
            case ')': return make_token(TOK_RPAREN);
            case '{': return make_token(TOK_LBRACE);
            case '}': return make_token(TOK_RBRACE);
            case ':': return make_token(TOK_COLON);
            case '=': return make_token(TOK_ASSIGN);
            case '<': if (src[pos] == '=') { pos++; return make_token(TOK_LE); } return make_token(TOK_LT);
            case '>': if (src[pos] == '=') { pos++; return make_token(TOK_GE); } return make_token(TOK_GT);
            default: break;
        }
    }
    return make_token(TOK_EOF);
}

#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "Uso: %s <archivo.cons>\n", argv[0]); return 1; }
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror("fopen"); return 1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *source = malloc(size + 1);
    if (fread(source, 1, size, f) != (size_t)size) { /* ignore */ }
    source[size] = '\0';
    fclose(f);
    lexer_init(source);
    Node *program = parse_program();
    fprintf(stderr, "AST: %p\n", (void*)program->body);
    codegen(program);
    free(source);
    return 0;
}

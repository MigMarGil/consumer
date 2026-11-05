#include "codegen.h"
#include <llvm-c/Core.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/Analysis.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static LLVMContextRef ctx;
static LLVMModuleRef mod;
static LLVMBuilderRef builder;
static LLVMBasicBlockRef entry_bb;
static LLVMValueRef main_fn;
static LLVMValueRef *var_allocas;
static char **var_names;
static int var_count;

static LLVMValueRef lookup_var(const char *name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(var_names[i], name) == 0) return var_allocas[i];
    return NULL;
}

static void declare_var(const char *name) {
    if (lookup_var(name)) return;
    LLVMBasicBlockRef cur = LLVMGetInsertBlock(builder);
    LLVMPositionBuilderAtEnd(builder, entry_bb);
    LLVMValueRef alloca = LLVMBuildAlloca(builder, LLVMDoubleTypeInContext(ctx), name);
    LLVMPositionBuilderAtEnd(builder, cur);
    var_allocas[var_count] = alloca;
    var_names[var_count] = strdup(name);
    var_count++;
}

static LLVMValueRef get_var(const char *name) {
    LLVMValueRef v = lookup_var(name);
    if (v) return v;
    declare_var(name);
    return lookup_var(name);
}

static void scan_vars(Node *node) {
    while (node) {
        if (node->type == NODE_LET && node->name) declare_var(node->name);
        if (node->body) scan_vars(node->body);
        if (node->else_body) scan_vars(node->else_body);
        if (node->left) scan_vars(node->left);
        if (node->right) scan_vars(node->right);
        node = node->next;
    }
}

static LLVMValueRef codegen_expr(Node *node) {
    if (!node) return LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0);
    switch (node->type) {
        case NODE_NUMBER: return LLVMConstReal(LLVMDoubleTypeInContext(ctx), node->num_val);
        case NODE_VAR: {
            LLVMValueRef a = lookup_var(node->name);
            if (!a) return LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0);
            return LLVMBuildLoad2(builder, LLVMDoubleTypeInContext(ctx), a, node->name);
        }
        case NODE_BINARY: {
            LLVMValueRef l = codegen_expr(node->left);
            LLVMValueRef r = codegen_expr(node->right);
            switch (node->op) {
                case TOK_PLUS:  return LLVMBuildFAdd(builder, l, r, "add");
                case TOK_MINUS: return LLVMBuildFSub(builder, l, r, "sub");
                case TOK_STAR:  return LLVMBuildFMul(builder, l, r, "mul");
                case TOK_SLASH: return LLVMBuildFDiv(builder, l, r, "div");
                case TOK_PERCENT: {
                    LLVMValueRef fmod_fn = LLVMGetNamedFunction(mod, "fmod");
                    if (!fmod_fn) {
                        LLVMTypeRef ps[] = { LLVMDoubleTypeInContext(ctx), LLVMDoubleTypeInContext(ctx) };
                        fmod_fn = LLVMAddFunction(mod, "fmod", LLVMFunctionType(LLVMDoubleTypeInContext(ctx), ps, 2, 0));
                    }
                    LLVMValueRef args[] = { l, r };
                    return LLVMBuildCall2(builder, LLVMGlobalGetValueType(fmod_fn), fmod_fn, args, 2, "mod");
                }
                case TOK_LT: {
                    LLVMValueRef c = LLVMBuildFCmp(builder, LLVMRealOLT, l, r, "lt");
                    return LLVMBuildSelect(builder, c, LLVMConstReal(LLVMDoubleTypeInContext(ctx),1.0), LLVMConstReal(LLVMDoubleTypeInContext(ctx),0.0), "ltsel");
                }
                case TOK_GT: {
                    LLVMValueRef c = LLVMBuildFCmp(builder, LLVMRealOGT, l, r, "gt");
                    return LLVMBuildSelect(builder, c, LLVMConstReal(LLVMDoubleTypeInContext(ctx),1.0), LLVMConstReal(LLVMDoubleTypeInContext(ctx),0.0), "gtsel");
                }
                case TOK_LE: {
                    LLVMValueRef c = LLVMBuildFCmp(builder, LLVMRealOLE, l, r, "le");
                    return LLVMBuildSelect(builder, c, LLVMConstReal(LLVMDoubleTypeInContext(ctx),1.0), LLVMConstReal(LLVMDoubleTypeInContext(ctx),0.0), "lesel");
                }
                case TOK_GE: {
                    LLVMValueRef c = LLVMBuildFCmp(builder, LLVMRealOGE, l, r, "ge");
                    return LLVMBuildSelect(builder, c, LLVMConstReal(LLVMDoubleTypeInContext(ctx),1.0), LLVMConstReal(LLVMDoubleTypeInContext(ctx),0.0), "gesel");
                }
            }
            return LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0);
        }
        default: return LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0);
    }
}

static LLVMBasicBlockRef codegen_stmt(Node *node, LLVMBasicBlockRef cur);

static LLVMBasicBlockRef codegen_block(Node *block, LLVMBasicBlockRef cur) {
    if (!block) return cur;
    for (Node *s = block->body; s; s = s->next) {
        LLVMPositionBuilderAtEnd(builder, cur);
        cur = codegen_stmt(s, cur);
    }
    return cur;
}

static LLVMBasicBlockRef codegen_stmt(Node *node, LLVMBasicBlockRef cur) {
    if (!node) return cur;
    switch (node->type) {
        case NODE_BLOCK:
            return codegen_block(node, cur);
        case NODE_LET: {
            LLVMValueRef val = codegen_expr(node->left);
            LLVMBuildStore(builder, val, get_var(node->name));
            break;
        }
        case NODE_PRINT: {
            LLVMValueRef v = codegen_expr(node->left);
            LLVMValueRef p = LLVMGetNamedFunction(mod, "printf");
            if (!p) {
                LLVMTypeRef ps[] = { LLVMPointerType(LLVMInt8TypeInContext(ctx),0) };
                p = LLVMAddFunction(mod, "printf", LLVMFunctionType(LLVMInt32TypeInContext(ctx), ps, 1, 1));
            }
            LLVMValueRef fmt = LLVMBuildGlobalStringPtr(builder, "%g\n", "fmt");
            LLVMValueRef args[] = { fmt, v };
            LLVMBuildCall2(builder, LLVMGlobalGetValueType(p), p, args, 2, "");
            break;
        }
        case NODE_WHILE: {
            LLVMBasicBlockRef cb = LLVMAppendBasicBlockInContext(ctx, main_fn, "wcond");
            LLVMBasicBlockRef bb = LLVMAppendBasicBlockInContext(ctx, main_fn, "wbody");
            LLVMBasicBlockRef eb = LLVMAppendBasicBlockInContext(ctx, main_fn, "wend");
            LLVMBuildBr(builder, cb);
            LLVMPositionBuilderAtEnd(builder, cb);
            LLVMValueRef cond = codegen_expr(node->left);
            LLVMValueRef z = LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0);
            LLVMValueRef cmp = LLVMBuildFCmp(builder, LLVMRealONE, cond, z, "wcmp");
            LLVMBuildCondBr(builder, cmp, bb, eb);
            LLVMPositionBuilderAtEnd(builder, bb);
            LLVMBasicBlockRef body_end = codegen_block(node->body, bb);
            LLVMPositionBuilderAtEnd(builder, body_end);
            if (!LLVMGetBasicBlockTerminator(body_end))
                LLVMBuildBr(builder, cb);
            LLVMPositionBuilderAtEnd(builder, eb);
            return eb;
        }
        case NODE_IF: {
            LLVMValueRef cond = codegen_expr(node->left);
            LLVMValueRef z = LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0);
            LLVMValueRef cmp = LLVMBuildFCmp(builder, LLVMRealONE, cond, z, "ifcmp");
            LLVMBasicBlockRef tb = LLVMAppendBasicBlockInContext(ctx, main_fn, "then");
            LLVMBasicBlockRef eb2 = LLVMAppendBasicBlockInContext(ctx, main_fn, "else");
            LLVMBasicBlockRef mb = LLVMAppendBasicBlockInContext(ctx, main_fn, "merge");
            LLVMBuildCondBr(builder, cmp, tb, eb2);
            LLVMPositionBuilderAtEnd(builder, tb);
            LLVMBasicBlockRef then_end = codegen_block(node->body, tb);
            LLVMPositionBuilderAtEnd(builder, then_end);
            if (!LLVMGetBasicBlockTerminator(then_end))
                LLVMBuildBr(builder, mb);
            LLVMPositionBuilderAtEnd(builder, eb2);
            LLVMBasicBlockRef else_end = eb2;
            if (node->else_body)
                else_end = codegen_block(node->else_body, eb2);
            LLVMPositionBuilderAtEnd(builder, else_end);
            if (!LLVMGetBasicBlockTerminator(else_end))
                LLVMBuildBr(builder, mb);
            LLVMPositionBuilderAtEnd(builder, mb);
            return mb;
        }
        default:
            codegen_expr(node);
            break;
    }
    return LLVMGetInsertBlock(builder);
}

void codegen(Node *program) {
    ctx = LLVMContextCreate();
    mod = LLVMModuleCreateWithNameInContext("consumer", ctx);
    builder = LLVMCreateBuilderInContext(ctx);
    main_fn = LLVMAddFunction(mod, "main", LLVMFunctionType(LLVMVoidTypeInContext(ctx), NULL, 0, 0));
    entry_bb = LLVMAppendBasicBlockInContext(ctx, main_fn, "entry");
    LLVMPositionBuilderAtEnd(builder, entry_bb);
    var_allocas = calloc(1024, sizeof(LLVMValueRef));
    var_names = calloc(1024, sizeof(char*));
    var_count = 0;
    scan_vars(program);
    LLVMBasicBlockRef last = codegen_block(program, entry_bb);
    LLVMPositionBuilderAtEnd(builder, last);
    if (!LLVMGetBasicBlockTerminator(last))
        LLVMBuildRetVoid(builder);
    LLVMVerifyModule(mod, LLVMAbortProcessAction, NULL);
    LLVMExecutionEngineRef engine; char *error = NULL;
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    if (LLVMCreateExecutionEngineForModule(&engine, mod, &error) != 0) {
        fprintf(stderr, "JIT error: %s\n", error); return;
    }
    uint64_t addr = LLVMGetFunctionAddress(engine, "main");
    ((void (*)(void))addr)();
    LLVMDisposeExecutionEngine(engine);
    LLVMDisposeBuilder(builder);
    LLVMContextDispose(ctx);
}

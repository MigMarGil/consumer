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
static LLVMValueRef *var_allocas;
static char **var_names;
static int var_count;

static LLVMValueRef lookup_var(const char *name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(var_names[i], name) == 0) return var_allocas[i];
    return NULL;
}

static LLVMValueRef get_or_create_var(const char *name) {
    LLVMValueRef v = lookup_var(name);
    if (v) return v;
    LLVMValueRef alloca = LLVMBuildAlloca(builder, LLVMDoubleTypeInContext(ctx), name);
    var_allocas[var_count] = alloca;
    var_names[var_count] = strdup(name);
    var_count++;
    return alloca;
}

static LLVMValueRef codegen_expr(Node *node) {
    if (!node) return LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0);
    switch (node->type) {
        case NODE_NUMBER:
            return LLVMConstReal(LLVMDoubleTypeInContext(ctx), node->num_val);
        case NODE_VAR: {
            LLVMValueRef alloca = lookup_var(node->name);
            if (!alloca) return LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0);
            return LLVMBuildLoad2(builder, LLVMDoubleTypeInContext(ctx), alloca, node->name);
        }
        case NODE_BINARY: {
            LLVMValueRef l = codegen_expr(node->left);
            LLVMValueRef r = codegen_expr(node->right);
            switch (node->op) {
                case TOK_PLUS:  return LLVMBuildFAdd(builder, l, r, "add");
                case TOK_MINUS: return LLVMBuildFSub(builder, l, r, "sub");
                case TOK_STAR:  return LLVMBuildFMul(builder, l, r, "mul");
                case TOK_SLASH: return LLVMBuildFDiv(builder, l, r, "div");
                case TOK_LT: {
                    LLVMValueRef c = LLVMBuildFCmp(builder, LLVMRealOLT, l, r, "lt");
                    return LLVMBuildSelect(builder, c,
                        LLVMConstReal(LLVMDoubleTypeInContext(ctx), 1.0),
                        LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0), "ltsel");
                }
                case TOK_GT: {
                    LLVMValueRef c = LLVMBuildFCmp(builder, LLVMRealOGT, l, r, "gt");
                    return LLVMBuildSelect(builder, c,
                        LLVMConstReal(LLVMDoubleTypeInContext(ctx), 1.0),
                        LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0), "gtsel");
                }
            }
            return LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0);
        }
        default:
            return LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0);
    }
}

static void codegen_stmt(Node *node) {
    while (node) {
        switch (node->type) {
            case NODE_BLOCK:
                codegen_stmt(node->body);
                break;
            case NODE_LET: {
                LLVMValueRef val = codegen_expr(node->left);
                LLVMValueRef alloca = get_or_create_var(node->name);
                LLVMBuildStore(builder, val, alloca);
                break;
            }
            case NODE_PRINT: {
                LLVMValueRef v = codegen_expr(node->left);
                LLVMValueRef printf_fn = LLVMGetNamedFunction(mod, "printf");
                if (!printf_fn) {
                    LLVMTypeRef params[] = { LLVMPointerType(LLVMInt8TypeInContext(ctx), 0) };
                    LLVMTypeRef ft = LLVMFunctionType(LLVMInt32TypeInContext(ctx), params, 1, 1);
                    printf_fn = LLVMAddFunction(mod, "printf", ft);
                }
                LLVMValueRef fmt = LLVMBuildGlobalStringPtr(builder, "%g\n", "fmt");
                LLVMValueRef args[] = { fmt, v };
                LLVMBuildCall2(builder, LLVMGlobalGetValueType(printf_fn), printf_fn, args, 2, "");
                break;
            }
            case NODE_WHILE: {
                LLVMValueRef parent = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
                LLVMBasicBlockRef cond_bb = LLVMAppendBasicBlockInContext(ctx, parent, "wcond");
                LLVMBasicBlockRef body_bb = LLVMAppendBasicBlockInContext(ctx, parent, "wbody");
                LLVMBasicBlockRef end_bb  = LLVMAppendBasicBlockInContext(ctx, parent, "wend");
                LLVMBuildBr(builder, cond_bb);
                LLVMPositionBuilderAtEnd(builder, cond_bb);
                LLVMValueRef cond = codegen_expr(node->left);
                LLVMValueRef zero = LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0);
                LLVMValueRef cmp = LLVMBuildFCmp(builder, LLVMRealONE, cond, zero, "wcmp");
                LLVMBuildCondBr(builder, cmp, body_bb, end_bb);
                LLVMPositionBuilderAtEnd(builder, body_bb);
                codegen_stmt(node->body);
                LLVMBuildBr(builder, cond_bb);
                LLVMPositionBuilderAtEnd(builder, end_bb);
                break;
            }
            case NODE_IF: {
                LLVMValueRef parent = LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder));
                LLVMValueRef cond = codegen_expr(node->left);
                LLVMValueRef zero = LLVMConstReal(LLVMDoubleTypeInContext(ctx), 0.0);
                LLVMValueRef cmp = LLVMBuildFCmp(builder, LLVMRealONE, cond, zero, "ifcmp");
                LLVMBasicBlockRef then_bb = LLVMAppendBasicBlockInContext(ctx, parent, "then");
                LLVMBasicBlockRef else_bb = LLVMAppendBasicBlockInContext(ctx, parent, "else");
                LLVMBasicBlockRef merge_bb = LLVMAppendBasicBlockInContext(ctx, parent, "merge");
                LLVMBuildCondBr(builder, cmp, then_bb, else_bb);
                LLVMPositionBuilderAtEnd(builder, then_bb);
                codegen_stmt(node->body);
                LLVMBuildBr(builder, merge_bb);
                LLVMPositionBuilderAtEnd(builder, else_bb);
                if (node->else_body) codegen_stmt(node->else_body);
                LLVMBuildBr(builder, merge_bb);
                LLVMPositionBuilderAtEnd(builder, merge_bb);
                break;
            }
            case NODE_RETURN: {
                LLVMValueRef v = codegen_expr(node->left);
                LLVMBuildRet(builder, v);
                return;
            }
            default:
                codegen_expr(node);
                break;
        }
        node = node->next;
    }
}

void codegen(Node *program) {
    ctx = LLVMContextCreate();
    mod = LLVMModuleCreateWithNameInContext("consumer", ctx);
    builder = LLVMCreateBuilderInContext(ctx);

    LLVMTypeRef ft = LLVMFunctionType(LLVMVoidTypeInContext(ctx), NULL, 0, 0);
    LLVMValueRef main_fn = LLVMAddFunction(mod, "main", ft);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(ctx, main_fn, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    var_allocas = calloc(1024, sizeof(LLVMValueRef));
    var_names = calloc(1024, sizeof(char*));
    var_count = 0;

    codegen_stmt(program->body);
    LLVMBuildRetVoid(builder);

    LLVMVerifyModule(mod, LLVMAbortProcessAction, NULL);

    LLVMExecutionEngineRef engine;
    char *error = NULL;
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    if (LLVMCreateExecutionEngineForModule(&engine, mod, &error) != 0) {
        fprintf(stderr, "JIT error: %s\n", error);
        return;
    }

    uint64_t addr = LLVMGetFunctionAddress(engine, "main");
    void (*fn)(void) = (void (*)(void))addr;
    fn();

    LLVMDisposeExecutionEngine(engine);
    LLVMDisposeBuilder(builder);
    LLVMContextDispose(ctx);
}

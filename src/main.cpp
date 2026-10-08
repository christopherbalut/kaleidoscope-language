#include "ast.hpp"
#include "codegen.hpp"
#include "lexer.hpp"
#include "parser.hpp"

#include <cstdio>
#include <llvm/Support/Error.h>
#include <memory>

using namespace llvm;

static ExitOnError ExitOnErr;

static void HandleDefinition() {
    if (auto FnAST = ParseDefinition()) {
        if (auto *FnIR = FnAST->codegen()) {
            fprintf(stderr, "Read function definition:");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else {
        // Skip token for error recovery.
        GetNextToken();
    }
}

static void HandleExtern() {
    if (auto ProtoAST = ParseExtern()) {
        if (auto *FnIR = ProtoAST->codegen()) {
            fprintf(stderr, "Read extern: ");
            FnIR->print(errs());
            fprintf(stderr, "\n");
        }
    } else {
        // Skip token for error recovery.
        GetNextToken();
    }
}

static void HandleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (auto FnAST = ParseTopLevelExpr()) {
        if (FnAST->codegen()) {
            auto RT = TheJIT->getMainJITDylib().createResourceTracker();

            auto TSM = llvm::orc::ThreadSafeModule(std::move(TheModule),
                                                   std::move(TheContext));

            ExitOnErr(TheJIT->addModule(std::move(TSM), RT));

            InitializeModuleAndManagers();

            auto ExprSymbol = ExitOnErr(TheJIT->lookup("__anon_expr"));

            double (*FP)() = ExprSymbol.toPtr<double (*)()>();

            fprintf(stderr, "Evaluated to %f\n", FP());

            ExitOnErr(RT->remove());
        }
    } else {
        // Skip token for error recovery.
        GetNextToken();
    }
} // <-- HandleTopLevelExpression ends HERE
/// top ::= definition | external | expression | ';'
static void MainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (CurTok) {
        case tok_eof:
            return;
        case ';': // ignore top-level semicolons.
            GetNextToken();
            break;
        case tok_def:
            HandleDefinition();
            break;
        case tok_extern:
            HandleExtern();
            break;
        default:
            HandleTopLevelExpression();
            break;
        }
    }
}

int main() {

    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();

    // Install standard BinopPrecedence
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 30;
    BinopPrecedence['*'] = 40;

    // Prime the first token.
    fprintf(stderr, "ready> ");
    GetNextToken();

    TheJIT = ExitOnErr(orc::KaleidoscopeJIT::Create());

    InitializeModuleAndManagers();

    // Run the main "interpreter loop" now.
    MainLoop();

    TheModule->print(llvm::errs(), nullptr);

    return 0;
}

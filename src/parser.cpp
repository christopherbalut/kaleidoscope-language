#include "parser.hpp"
#include "lexer.hpp"
#include <memory>
#include <vector>

int CurTok;
std::map<char, int> BinopPrecedence;
// read token from the lexer and updates the current token

int GetNextToken() { return CurTok = gettok(); }

std::unique_ptr<ExprAST> LogError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}

llvm::Value *LogErrorV(const char *Str) {
    LogError(Str);
    return nullptr;
}

std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    GetNextToken();
    return Result;
}

std::unique_ptr<ExprAST> ParseParenExpr() {
    GetNextToken();

    auto V = ParseExpression();
    if (!V) {
        return nullptr;
    }

    if (CurTok != ')') {
        return LogError("Expecting closing ')', terminating...");
    }

    GetNextToken();
    return V;
}

std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName{IdentifierStr};

    GetNextToken();

    if (CurTok != '(') {
        return std::make_unique<VariableExprAST>(std::move(IdName));
    }

    GetNextToken(); // for '('
    std::vector<std::unique_ptr<ExprAST>> Args;

    if (CurTok != ')') {
        while (true) {
            if (auto Arg{ParseExpression()}) {
                Args.emplace_back(std::move(Arg));
            } else {
                return nullptr;
            }

            if (CurTok == ')') {
                break;
            }

            if (CurTok != ',') {
                return LogError("Expecting comma to seperate function input "
                                "parameters, terminating...");
            }
            GetNextToken();
        }
    }

    GetNextToken(); // for ')'
    return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

std::unique_ptr<ExprAST> ParseIfExpr() {
    GetNextToken(); // this will eat the if string

    auto Cond{ParseExpression()};

    if (!Cond) {
        return nullptr;
    }

    if (CurTok != tok_then) {
        return LogError("expected then statement, terminating...");
    }
    GetNextToken();

    auto Then{ParseExpression()};
    if (!Then) {
        return nullptr;
    }

    if (CurTok != tok_else) {
        return LogError("expected else, terminating...");
    }

    GetNextToken();

    auto Else{ParseExpression()};
    if (!Else) {
        return nullptr;
    }
    return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
                                       std::move(Else));
}

std::unique_ptr<ExprAST> ParseForExpr() {
    GetNextToken(); // eat the for.

    if (CurTok != tok_identifier)
        return LogError("expected identifier after for");

    std::string IdName{IdentifierStr};
    GetNextToken(); // eat identifier.

    if (CurTok != '=')
        return LogError("expected '=' after for");
    GetNextToken(); // eat '='.

    auto Start{ParseExpression()};
    if (!Start)
        return nullptr;
    if (CurTok != ',')
        return LogError("expected ',' after for start value");
    GetNextToken();

    auto End{ParseExpression()};
    if (!End)
        return nullptr;

    // The step value is optional.
    std::unique_ptr<ExprAST> Step;
    if (CurTok == ',') {
        GetNextToken();
        Step = ParseExpression();
        if (!Step)
            return nullptr;
    }

    if (CurTok != tok_in)
        return LogError("expected 'in' after for");
    GetNextToken(); // eat 'in'.

    auto Body{ParseExpression()};
    if (!Body)
        return nullptr;

    return std::make_unique<ForExprAST>(IdName, std::move(Start),
                                        std::move(End), std::move(Step),
                                        std::move(Body));
}

std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
    default:
        return LogError(
            "Unknown token when expecting an expression, terminating...");
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    case '(':
        return ParseParenExpr();
    case tok_if:
        return ParseIfExpr();
    case tok_for:
        return ParseForExpr();
    }
}

std::optional<int> GetTokPrecedence() {
    if (CurTok < 0 || CurTok > 127) {
        return std::nullopt;
    }

    if (const auto it = BinopPrecedence.find(static_cast<char>(CurTok));
        it != BinopPrecedence.end() && it->second > 0) {
        return it->second;
    }
    return std::nullopt;
}

std::unique_ptr<ExprAST> ParseExpression() {

    auto LHS = ParsePrimary();
    if (!LHS) {
        return nullptr;
    }

    return ParseBinOpRHS(0, std::move(LHS));
}

std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                       std::unique_ptr<ExprAST> LHS) {
    while (true) {
        const auto Precedence{GetTokPrecedence()};

        if (!Precedence || *Precedence < ExprPrec) {
            return LHS;
        }

        const int TokPrec{*Precedence};
        const char BinOp{static_cast<char>(CurTok)};

        auto RHS{ParsePrimary()};
        if (!RHS) {
            return nullptr;
        }

        const auto NextPrec{GetTokPrecedence()};
        if (NextPrec && TokPrec < *NextPrec) {
            RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
            if (!RHS) {
                return nullptr;
            }
        }
        LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS),
                                              std::move(RHS));
    }
}

std::unique_ptr<PrototypeAST> ParsePrototype() {
    if (CurTok != tok_identifier) {
        return LogErrorP("Expected function name in prototype");
    }

    std::string FnName{IdentifierStr};
    GetNextToken();

    if (CurTok != '(') {
        return LogErrorP("Expected open parentheses");
    }

    std::vector<std::string> ArgNames;
    while (GetNextToken() == tok_identifier) {
        ArgNames.emplace_back(IdentifierStr);
    }

    if (CurTok != ')') {
        return LogErrorP("Expected a closing parentheses");
    }

    GetNextToken();

    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

std::unique_ptr<FunctionAST> ParseDefinition() {
    GetNextToken();
    auto Proto{ParsePrototype()};
    if (!Proto) {
        return nullptr;
    }

    if (auto E{ParseExpression()}) {
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

std::unique_ptr<PrototypeAST> ParseExtern() {
    GetNextToken();
    return ParsePrototype();
}

std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
    if (auto E{ParseExpression()}) {
        auto Proto{std::make_unique<PrototypeAST>("__anon_expr",
                                                  std::vector<std::string>())};
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

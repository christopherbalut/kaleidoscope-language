#pragma once

#include "ast.hpp"
#include <map>
#include <memory>
#include <optional>

extern int CurTok;
extern std::map<char, int> BinopPrecedence;
int GetNextToken();

std::unique_ptr<ExprAST> LogError(const char *Str);
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str);
llvm::Value *LogErrorV(const char *Str);

[[nodiscard]] std::unique_ptr<ExprAST> ParseNumberExpr();
[[nodiscard]] std::unique_ptr<ExprAST> ParseParenExpr();
[[nodiscard]] std::unique_ptr<ExprAST> ParseIdentifierExpr();

[[nodiscard]] std::unique_ptr<ExprAST> ParsePrimary();

[[nodiscard]] std::unique_ptr<ExprAST> ParseExpression();

[[nodiscard]] std::optional<int> GetTokPrecedence();

[[nodiscard]] std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                       std::unique_ptr<ExprAST> LHS);

[[nodiscard]] std::unique_ptr<PrototypeAST> ParsePrototype();
[[nodiscard]] std::unique_ptr<FunctionAST> ParseDefinition();
[[nodiscard]] std::unique_ptr<PrototypeAST> ParseExtern();
[[nodiscard]] std::unique_ptr<FunctionAST> ParseTopLevelExpr();

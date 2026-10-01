#include "ast.hpp"
#include "codegen.hpp"
#include "parser.hpp"
#include <llvm/IR/Instructions.h>
#include <map>
#include <memory>
#include <utility>

using llvm::Builder;
using llvm::Function;
using llvm::Value;

std::map<std::string, Value *>
    NamedValues; // keeps track of current values that are defined in different
                 // scopes
NumberExprAST::NumberExprAST(double Val) : Val(Val) {}

double NumberExprAST::GetValue() const { return Val; } // temp for -Werror

Value *NumberExprAST::codegen() {
    Value *fpconst{llvm::ConstantFP::get(*TheContext, llvm::APFloat(Val))};
    if (!fpconst) {
        return nullptr;
    }
    return fpconst;
}

VariableExprAST::VariableExprAST(const std::string &Name) : Name(Name) {}

Value *VariableExprAST::codegen() {
    Value *V{NamedValues[Name]};

    if (!V) {
        return LogErrorV("Unkown variable name");
    }

    return V;
}

BinaryExprAST::BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                             std::unique_ptr<ExprAST> RHS)
    : Op(Op), RHS(std::move(RHS)), LHS(std::move(LHS)) {}
// std::move since unique_ptr's cannot be copied

char BinaryExprAST::GetOp() const { return Op; } // temp for -Werror

// case of 1 + 2
// we recurse on the left, so we call
Value *BinaryExprAST::codegen() {
    Value *L{LHS->codegen()};
    Value *R{RHS->codegen()};

    if (!L || !R) {
        return nullptr;
    }

    switch (Op) {
    case '+':
        return Builder->CreateFAdd(L, R, "addtmp");
        break;
    case '-':
        return Builder->CreateFSub(L, R, "subtmp");
        break;
    case '*':
        return Builder->CreateFMul(L, R, "multtmp");
        break;
    case '<':
        // floating point comparison creates i32
        L = Builder->CreateFCmpULT(L, R, "cmptmp");
        return Builder->CreateUIToFP(L, llvm::Type::getDoubleTy(*TheContext));
        break;
    default:
        // i probably have to log the error somewhere
        return LogErrorV("invalid binary operator");
    }
}

CallExprAST::CallExprAST(const std::string &Callee,
                         std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}

Value *CallExprAST::codegen() {
    // lookup name in the gobal module table
    Function *CalleeF{TheModule->getFunction(Callee)};
    if (!CalleeF) { // function does not exist
        return nullptr;
    }

    if (CalleeF->arg_size() != Args.size()) {
        return LogErrorV("Incorrect number of arguments passed");
    }

    std::vector<Value *> ArgsV;
    for (unsigned i{}; i < Args.size(); i++) {
        ArgsV.emplace_back(Args[i]->codegen());
    }

    if (!ArgsV.back()) { // if most recent arg failed to codegen
        return nullptr;
    }

    return Builder->CreateCall(CalleeF, ArgsV, "call");
}

IfExprAST::IfExprAST(std::unique_ptr<ExprAST> Cond,
                     std::unique_ptr<ExprAST> Then,
                     std::unique_ptr<ExprAST> Else)
    : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

Value *IfExprAST::codegen() {
    Value *CondV{Cond->codegen()};
    if (!CondV) {
        return nullptr;
    }

    // converts the condition to a bool by comparing non equal  to 0.0
    CondV = Builder->CreateFCmpONE(
        CondV, llvm::ConstantFP::get(*TheContext, llvm::APFloat(0.0)),
        "ifcondition");

    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    // Create blocks for the then and else case, Isnert the 'then' block at teh
    // end of the function
    llvm::BasicBlock *ThenBB{
        llvm::BasicBlock::Create(*TheContext, "then", TheFunction)};
    llvm::BasicBlock *ElseBB{llvm::BasicBlock::Create(*TheContext, "else")};
    llvm::BasicBlock *MergeBB{llvm::BasicBlock::Create(*TheContext, "ifcont")};

    //
    Builder->CreateCondBr(CondV, ThenBB, ElseBB);

    // emit else block
    TheFunction->insert(TheFunction->end(), ElseBB);
    Builder->SetInsertPoint(ElseBB);

    Value *ElseV{Else->codegen()};
    if (!ElseV) {
        return nullptr;
    }

    Builder->CreateBr(MergeBB);

    Builder->SetInsertPoint(ThenBB);

    Value *ThenV{Then->codegen()};
    if (!ThenV)
        return nullptr;

    ElseBB = Builder->GetInsertBlock();

    TheFunction->insert(TheFunction->end(), MergeBB);
    Builder->SetInsertPoint(MergeBB);
    llvm::PHINode *PN =
        Builder->CreatePHI(llvm::Type::getDoubleTy(*TheContext), 2, "iftmp");

    PN->addIncoming(ThenV, ThenBB);
    PN->addIncoming(ElseV, ElseBB);
    return PN;
}

Value *ForExprAST::codegen() {
    // Emit the start code first, without 'variable' in scope.
    Value *StartVal{Start->codegen()};
    if (!StartVal)
        return nullptr;

    // Make the new basic block for the loop header, inserting after current
    // block.
    Function *TheFunction{Builder->GetInsertBlock()->getParent()};
    BasicBlock *PreheaderBB{Builder->GetInsertBlock()};
    BasicBlock *LoopBB{BasicBlock::Create(*TheContext, "loop", TheFunction)};

    // Insert an explicit fall through from the current block to the LoopBB.
    Builder->CreateBr(LoopBB);

    // Start insertion in LoopBB.
    Builder->SetInsertPoint(LoopBB);

    // Start the PHI node with an entry for Start.
    PHINode *Variable{
        Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, VarName)};
    Variable->addIncoming(StartVal, PreheaderBB);

    // Within the loop, the variable is defined equal to the PHI node. If it
    // shadows an existing variable, we have to restore it, so save it now.
    Value *OldVal{NamedValues[VarName]};
    NamedValues[VarName] = Variable;

    // Emit the body of the loop. This, like any other expr, can change the
    // current BB. Note that we ignore the value computed by the body, but don't
    // allow an error.
    if (!Body->codegen())
        return nullptr;

    // Emit the step value.
    Value *StepVal{nullptr};
    if (Step) {
        StepVal = Step->codegen();
        if (!StepVal)
            return nullptr;
    } else {
        // If not specified, use 1.0.
        StepVal = ConstantFP::get(*TheContext, APFloat(1.0));
    }

    Value *NextVar{Builder->CreateFAdd(Variable, StepVal, "nextvar")};

    // Compute the end condition.
    Value *EndCond{End->codegen()};
    if (!EndCond)
        return nullptr;

    // Convert condition to a bool by comparing non-equal to 0.0.
    EndCond = Builder->CreateFCmpONE(
        EndCond, ConstantFP::get(*TheContext, APFloat(0.0)), "loopcond");

    // Create the "after loop" block and insert it.
    BasicBlock *LoopEndBB{Builder->GetInsertBlock()};
    BasicBlock *AfterBB{
        BasicBlock::Create(*TheContext, "afterloop", TheFunction)};

    // Insert the conditional branch into the end of LoopEndBB.
    Builder->CreateCondBr(EndCond, LoopBB, AfterBB);

    // Any new code will be inserted in AfterBB.
    Builder->SetInsertPoint(AfterBB);

    // Add a new entry to the PHI node for the backedge.
    Variable->addIncoming(NextVar, LoopEndBB);

    // Restore the unshadowed variable.
    if (OldVal)
        NamedValues[VarName] = OldVal;
    else
        NamedValues.erase(VarName);

    // for expr always returns 0.0.
    return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

PrototypeAST::PrototypeAST(const std::string &Name,
                           std::vector<std::string> Args)
    : Name(Name), Args(std::move(Args)) {}

const std::string &PrototypeAST::GetName() const { return Name; }

llvm::Function *PrototypeAST::codegen() {
    std::vector<llvm::Type *> Doubles(
        Args.size(), // creating a vector for the Module
        llvm::Type::getDoubleTy(*TheContext));

    // return type, parameters, not variadic
    llvm::FunctionType *FT{llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*TheContext), Doubles, false)};

    llvm::Function *F{
        Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get())};

    unsigned Idx{0};
    for (auto &Arg : F->args()) {
        Arg.setName(Args[Idx++]);
    }

    return F;
}

FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                         std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}

llvm::Function *FunctionAST::codegen() {
    auto &P{*Proto};
    FunctionProtos[Proto->GetName()] = std::move(Proto);
    llvm::Function *TheFunction{getFunction(P.GetName())};

    if (!TheFunction) {
        return nullptr;
    }

    // create a new basic block to start insertion into it
    llvm::BasicBlock *BB{
        llvm::BasicBlock::Create(*TheContext, "entry", TheFunction)};
    Builder->SetInsertPoint(BB);

    // record the function arguments in the NamedValues map
    NamedValues.clear();
    for (auto &Arg : TheFunction->args()) {
        NamedValues[std::string(Arg.getName())] = &Arg;
    }

    //
    if (Value * RetVal{Body->codegen()}) {
        Builder->CreateRet(RetVal);
        llvm::verifyFunction(*TheFunction);

        // Optimize the function
        TheFPM->run(*TheFunction, *TheFAM);

        return TheFunction;
    }

    TheFunction->eraseFromParent();
    return nullptr;
}

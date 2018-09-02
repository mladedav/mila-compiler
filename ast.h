#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace llvm;

#ifndef AST_H
#define AST_H

namespace mila
{
    class ExprAST 
    {
        public:
            virtual ~ExprAST() {}
            virtual Value *codegen() = 0;
            virtual void print() const = 0;
    };

    class ExprListAST : public ExprAST
    {
        std::vector<std::unique_ptr<ExprAST>> Nodes;

        public:
        ExprListAST(std::vector<std::unique_ptr<ExprAST>> Nodes) : Nodes(std::move(Nodes)) {}
        Value *codegen() override;
        void print() const override;
    };

    /// NumberExprAST - Expression class for numeric literals like "1".
    class NumberExprAST : public ExprAST 
    {
        int Val;

        public:
        NumberExprAST(int Val) : Val(Val) {}
        Value *codegen() override;
        void print() const override;
    };

    class ConstExprAST : public ExprAST
    {
        std::string Name;
        int Val;

        public:
        ConstExprAST(const std::string &Name, int Val) : Name(Name), Val(Val) {}
        Value *codegen() override;
        void print() const override;
    };

    class DeclareExprAST : public ExprAST
    {
        std::string Name;
        int Offset;
        int Length;

        public:
        DeclareExprAST(const std::string &Name, int Offset = 0, int Length = 0) : Name(Name), Offset(Offset), Length(Length) {}
        Value *codegen() override;
        void print() const override;
    };

    /// VariableExprAST - Expression class for referencing a variable, like "a".
    class VariableExprAST : public ExprAST 
    {
        protected:
        std::string Name;

        public:
        VariableExprAST(const std::string &Name) : Name(Name) {}
        Value *codegen() override;
        void print() const override;
        const std::string &getName() const { return Name; }
        virtual Value *alloca() const;
    };

    class ArrayExprAST : public VariableExprAST
    {
        std::unique_ptr<ExprAST> Index;

        public:
        ArrayExprAST(const std::string &Name, std::unique_ptr<ExprAST> Index) : VariableExprAST(Name), Index(std::move(Index)) {}
        Value *codegen() override;
        void print() const override;
        virtual Value *alloca() const;
    };

    enum OperEnum 
    {
        ASSIGN, // 0
        ADD,    // 1
        SUB,    // 2
        MULT,   // 3
        DIV,    // 4
        MOD,    // 5
        LT,     // 6
        LE,     // 7
        GT,     // 8
        GE,     // 9
        EQ,     // 10
        NE,     // 11
        AND,    // 12
        OR,     // 13
    };

    int getPrecedence (OperEnum op);

    /// BinaryExprAST - Expression class for a binary operator.
    class BinaryExprAST : public ExprAST 
    {
        OperEnum Op;
        std::unique_ptr<ExprAST> LHS, RHS;

        public:
        BinaryExprAST(OperEnum Op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
            : Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
        Value *codegen() override;
        void print() const override;
    };

    /// IfExprAST - Expression class for if/then/else.
    class IfExprAST : public ExprAST
    {
        std::unique_ptr<ExprAST> Cond, Then, Else;

        public:
        IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
                std::unique_ptr<ExprAST> Else)
            : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

        Value *codegen() override;
        void print() const override;
    };

    /// ForExprAST - Expression class for for/in.
    class ForExprAST : public ExprAST
    {
        std::string VarName;
        std::unique_ptr<ExprAST> Start, End, Step, Body;

        public:
        ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
                std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
                std::unique_ptr<ExprAST> Body)
            : VarName(VarName), Start(std::move(Start)), End(std::move(End)),
            Step(std::move(Step)), Body(std::move(Body)) {}

        Value *codegen() override;
        void print() const override;
    };

    class WhileExprAST : public ExprAST
    {
        std::unique_ptr<ExprAST> Cond, Body;

        public:
        WhileExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Body)
            : Cond(std::move(Cond)), Body(std::move(Body)) {}
        Value *codegen() override;
        void print() const override;
    };

    /// CallExprAST - Expression class for function calls.
    class CallExprAST : public ExprAST 
    {
        std::string Callee;
        std::vector<std::unique_ptr<ExprAST>> Args;

        public:
        CallExprAST(const std::string &Callee,
                std::vector<std::unique_ptr<ExprAST>> Args)
            : Callee(Callee), Args(std::move(Args)) {}
        Value *codegen() override;
        void print() const override;
    };

    class LibraryExprAST : public ExprAST
    {
        std::string Name;
        std::string Arg;

        public:
        LibraryExprAST(const std::string &Name, const std::string &Arg) : Name(Name), Arg(Arg) {}
        Value *codegen() override;
        void print() const override;
    };

    class ReturnExprAST : public ExprAST
    {
        public:
        ReturnExprAST() {}
        Value *codegen() override;
        void print() const override;
    };

    /// PrototypeAST - This class represents the "prototype" for a function,
    /// which captures its name, and its argument names (thus implicitly the number
    /// of arguments the function takes).
    class PrototypeAST : public ExprAST
    {
        std::string Name;
        std::vector<std::string> Args;

        public:
        PrototypeAST(const std::string &Name, std::vector<std::string> Args)
            : Name(Name), Args(std::move(Args)) {}
        Function *codegen() override;
        void print() const override;
        const std::string &getName() const { return Name; }
    };

    /// FunctionAST - This class represents a function definition itself.
    class FunctionAST : public ExprAST
    {
        std::unique_ptr<PrototypeAST> Proto;
        std::vector<std::unique_ptr<ExprAST>> Body;

        public:
        FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                std::vector<std::unique_ptr<ExprAST>> Body)
            : Proto(std::move(Proto)), Body(std::move(Body)) {}
        Function *codegen() override;
        void print() const override;
    };

    //################################################################################
    AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          const std::string &VarName);

    extern LLVMContext TheContext;
    extern IRBuilder<> Builder;
    extern std::unique_ptr<Module> TheModule;
    extern std::map<std::string, Value *> NamedValues;
    extern std::map<std::string, Value *> ConstValues;
    //static std::unique_ptr<legacy::FunctionPassManager> TheFPM;
    extern std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
    extern std::map<std::string, Function *> Library;
    extern Function *main_func;
    extern BasicBlock *mainBlock;
};

#endif

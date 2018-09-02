// Most copied from LLVM's official tutorials about building Kaleidoscope language

//#include "llvm/ADT/APFloat.h"
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
#include "ast.h"
#include <iostream>

using namespace llvm;
using namespace mila;

namespace mila
{
    LLVMContext TheContext;
    std::unique_ptr<Module> TheModule ( make_unique<Module>("mila",TheContext) );
    IRBuilder<> Builder = IRBuilder<> ( TheContext );
    std::map<std::string, Value *> NamedValues;
    std::map<std::string, Value *> ConstValues;
    std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
    std::map<std::string, Function *> Library
    {
        std::make_pair ( "readln",
                cast<Function>(TheModule->getOrInsertFunction("readln",IntegerType::getInt32Ty(TheModule->getContext()),PointerType::getUnqual(IntegerType::getInt32Ty(TheModule->getContext()))))),
            std::make_pair ( "inc",
                    cast<Function>(TheModule->getOrInsertFunction("inc",IntegerType::getInt32Ty(TheModule->getContext()),PointerType::getUnqual(IntegerType::getInt32Ty(TheModule->getContext()))))),
            std::make_pair ( "dec",
                    cast<Function>(TheModule->getOrInsertFunction("dec",IntegerType::getInt32Ty(TheModule->getContext()),PointerType::getUnqual(IntegerType::getInt32Ty(TheModule->getContext())))))
    };

// Create main function
Function *main_func = cast<Function>(TheModule->
    getOrInsertFunction("main", IntegerType::getInt32Ty(TheModule->getContext()),
                  NULL));
    
// Create basic block and start inserting into it
BasicBlock *mainBlock = BasicBlock::Create(TheModule->getContext(), "main.0", main_func);
};

int mila::getPrecedence ( OperEnum op )
{
    switch (op)
    {
        case ASSIGN:
            throw ("Assign should not have any precedence");
        case ADD:
        case SUB:
            return 1;
        case MULT:
        case DIV:
        case MOD:
            return 2;
        case LT:
        case LE:
        case GT:
        case GE:
        case EQ:
        case NE:
            return 3;
        case OR:
        case AND:
            return 4;
        default:
            throw ("Unknown operator precedence");
    }
    return 0;
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

/// LogError* - These are little helper functions for error handling.
/*
   std::unique_ptr<ExprAST> LogError(const char *Str) {
   fprintf(stderr, "Error: %s\n", Str);
   return nullptr;
   }

   std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
   LogError(Str);
   return nullptr;
   }
   */


/*
   Value *LogErrorV(const char *Str) {
   LogError(Str);
   return nullptr;
   }
   */

Value * mila::NumberExprAST::codegen() 
{
    return ConstantInt::get(TheContext, APInt(32, Val, true));
}

Value * mila::ExprListAST::codegen()
{
    for (auto &expr : Nodes)
        if(expr)
            if(!expr->codegen())
                return nullptr;
     return Constant::getNullValue(Type::getInt32Ty(TheContext));
}

Value * mila::ConstExprAST::codegen()
{
    Value * alloca = ConstValues[Name];
    if (alloca)
        throw ("Constant redeclaration");
    alloca = Builder.CreateAlloca(Type::getInt32Ty(TheContext));
    Builder.CreateStore(ConstantInt::get(TheContext, APInt(32, Val, true)), alloca);
    ConstValues[Name] = alloca;
    return alloca;
}

Value * mila::DeclareExprAST::codegen()
{
    std::string sugar = Name + '/' + Builder.GetInsertBlock()->getParent()->getName().str();
    Value * alloca = NamedValues[sugar];
    if (alloca)
        throw ("Variable redeclaration.");

    if (Length)
    {
        alloca = Builder.CreateAlloca(Type::getInt32Ty(TheContext),
                                    ConstantInt::get(TheContext, APInt(32, Length, true)));
        alloca = Builder.CreateGEP(Type::getInt32Ty(TheContext), alloca, ConstantInt::get(TheContext, APInt(32, Offset, true)));
    }
    else
        alloca = Builder.CreateAlloca(Type::getInt32Ty(TheContext));
    NamedValues[sugar] = alloca;
    return alloca;
}

Value * mila::VariableExprAST::codegen() 
{
    std::string sugar = Name + '/' + Builder.GetInsertBlock()->getParent()->getName().str();
    // Look this variable up in the function.
    Value *V = NamedValues[sugar];
    if (!V)
        V = ConstValues[Name];
    if (!V)
        throw ("Unknown constant/variable name");
    return Builder.CreateLoad(V, Name.c_str());
}

Value * mila::ArrayExprAST::codegen()
{
    std::string sugar = Name + '/' + Builder.GetInsertBlock()->getParent()->getName().str();
    Value *V = NamedValues[sugar];
    if (!V)
        throw ("Unknown array name");
    auto ptr = Builder.CreateGEP(Type::getInt32Ty(TheContext), V, Index->codegen());
    return Builder.CreateLoad(ptr);
}

Value *BinaryExprAST::codegen() {
  // Special case '=' because we don't want to emit the LHS as an expression.
  if (Op == ASSIGN) {
    // Assignment requires the LHS to be an identifier.
    // This assume we're building without RTTI because LLVM builds that way by
    // default.  If you build LLVM with RTTI this can be changed to a
    // dynamic_cast for automatic error checking.
    VariableExprAST *LHSE = static_cast<VariableExprAST *>(LHS.get());
    if (!LHSE)
      throw("destination of '=' must be a variable");
    // Codegen the RHS.
    Value *Val = RHS->codegen();
    if (!Val)
      return nullptr;

    // Look up the name.
    //Value *Variable = NamedValues[LHSE->getName()];
    Value *Variable = LHSE->alloca();
    if (!Variable)
      throw("Unknown variable name");

    Builder.CreateStore(Val, Variable);
    return Val;
  }

  Value *L = LHS->codegen();
  Value *R = RHS->codegen();
  if (!L || !R)
    return nullptr;

  switch (Op) {
  case ADD:
    return Builder.CreateAdd(L, R, "addtmp");
  case SUB:
    return Builder.CreateSub(L, R, "subtmp");
  case MULT:
    return Builder.CreateMul(L, R, "multmp");
  case DIV:
    return Builder.CreateSDiv(L, R, "divtmp");
  case MOD:
    return Builder.CreateSRem(L, R, "modtmp");
  case LT:
    L = Builder.CreateICmpSLT(L, R, "lttmp");
    return Builder.CreateIntCast(L, Type::getInt32Ty(TheContext), true, "booltmp");
  case LE:
    L = Builder.CreateICmpSLE(L, R, "letmp");
    return Builder.CreateIntCast(L, Type::getInt32Ty(TheContext), true, "booltmp");
  case GT:
    L = Builder.CreateICmpSGT(L, R, "gttmp");
    return Builder.CreateIntCast(L, Type::getInt32Ty(TheContext), true, "booltmp");
  case GE:
    L = Builder.CreateICmpSGE(L, R, "getmp");
    return Builder.CreateIntCast(L, Type::getInt32Ty(TheContext), true, "booltmp");
  case EQ:
    L = Builder.CreateICmpEQ(L, R, "eqtmp");
    return Builder.CreateIntCast(L, Type::getInt32Ty(TheContext), true, "booltmp");
  case NE:
    L = Builder.CreateICmpNE(L, R, "netmp");
    return Builder.CreateIntCast(L, Type::getInt32Ty(TheContext), true, "booltmp");
  case AND:
    L = Builder.CreateICmpNE(
                L, ConstantInt::get(TheContext, APInt(32, 0, true)), "booltmp");
    R = Builder.CreateICmpNE(
                R, ConstantInt::get(TheContext, APInt(32, 0, true)), "booltmp");
    L = Builder.CreateAnd(L, R, "andtmp");
    return Builder.CreateIntCast(L, Type::getInt32Ty(TheContext), true, "booltmp");
  case OR:
    return Builder.CreateOr(L, R, "ortmp");
  default:
    throw ("Unknown operator");
    break;
  }

  return nullptr;
}

Value * mila::CallExprAST::codegen() 
{
    // Look up the name in the global module table.
    Function *CalleeF = TheModule->getFunction(Callee);
    if (!CalleeF)
        throw ("Unknown function referenced");

    // If argument mismatch error.
    if (CalleeF->arg_size() != Args.size())
        throw ("Incorrect # arguments passed");

    std::vector<Value *> ArgsV;
    for (unsigned i = 0, e = Args.size(); i != e; ++i) 
    {
        ArgsV.push_back(Args[i]->codegen());
        if (!ArgsV.back())
            return nullptr;
    }

    return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Value * mila::LibraryExprAST::codegen()
{
    std::string sugar = Arg + '/' + Builder.GetInsertBlock()->getParent()->getName().str();
    auto ptr = NamedValues [sugar];
    if (!ptr)
        return nullptr;
    auto func = Library [Name];
    return Builder.CreateCall(func, ptr);
}

Function * mila::PrototypeAST::codegen() 
{
    // Make the function type:  double(double,double) etc.
    std::vector<Type *> Ints(Args.size(), Type::getInt32Ty(TheContext));
    FunctionType *FT =
        FunctionType::get(Type::getInt32Ty(TheContext), Ints, false);

    Function *F =
        Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

    // Set names for all arguments.
    unsigned Idx = 0;
    for (auto &Arg : F->args())
        Arg.setName(Args[Idx++]);

    return F;
}

Function * mila::FunctionAST::codegen() 
{
    // First, check for an existing function from a previous 'extern' declaration.
    Function *TheFunction = TheModule->getFunction(Proto->getName());

    if (!TheFunction)
        TheFunction = Proto->codegen();

    if (!TheFunction)
        return nullptr;

    // Create a new basic block to start insertion into.
    BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
    Builder.SetInsertPoint(BB);

    // Record the function arguments in the NamedValues map.
    //NamedValues.clear();
    for (auto &Arg : TheFunction->args())
    {
        // Create an alloca for this variable.
        Value *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName());

        // Store the initial value int o the alloca.
        Builder.CreateStore(&Arg, Alloca);

        // Add arguments to variable symbol table.
        std::string sugar = Arg.getName().str() + '/' + Builder.GetInsertBlock()->getParent()->getName().str();
        NamedValues[sugar] = Alloca;
    }
    
    BasicBlock *Ret = BasicBlock::Create(TheContext, "return", TheFunction);

    for (unsigned i = 0 ; i < Body.size() ; i++)
    {
        if (i != Body.size() - 1)
            Body[i]->codegen();
        else if (Value *RetVal = Body[i]->codegen()) 
        {
            Builder.CreateBr(Ret);
            // Finish off the function.
            Builder.SetInsertPoint(Ret);
            Builder.CreateRet(RetVal);

            // Validate the generated code, checking for consistency.
            verifyFunction(*TheFunction);

            Builder.SetInsertPoint(mainBlock);
            return TheFunction;
        }
        else break;
    }

    // Error reading body, remove function.
    TheFunction->eraseFromParent();
    return nullptr;
}

Value * mila::ReturnExprAST::codegen()
{
    Function *TheFunction = TheModule->getFunction(Builder.GetInsertPoint()->getParent()->getName());
    std::string sugar = TheFunction->getName().str() + '/' + Builder.GetInsertBlock()->getParent()->getName().str();
    Value *Alloca = NamedValues[sugar];
    Value *Ret;
    if (Alloca)
        Ret = Builder.CreateLoad(Alloca);
    else
        Ret = ConstantInt::get(TheContext, APInt(32, 0, true));
    Builder.CreateRet(Ret);
    BasicBlock *Cont = BasicBlock::Create(TheContext, "dunno", TheFunction);
    Builder.SetInsertPoint(Cont);
    return ConstantInt::get(TheContext, APInt(32, 0, true));
}

// Output for-loop as:
//   var = alloca double
//   ...
//   start = startexpr
//   store start -> var
//   goto loop
// loop:
//   ...
//   bodyexpr
//   ...
// loopend:
//   step = stepexpr
//   endcond = endexpr
//
//   curvar = load var
//   nextvar = curvar + step
//   store nextvar -> var
//   br endcond, loop, endloop
// outloop:
Value * mila::ForExprAST::codegen() {
  Function *TheFunction = Builder.GetInsertBlock()->getParent();

  // Create an alloca for the variable in the entry block.
  //Value *Alloca = CreateEntryBlockAlloca(TheFunction, VarName);
    std::string sugar = VarName + '/' + Builder.GetInsertBlock()->getParent()->getName().str();
  Value *Alloca = NamedValues [sugar];
  if (!Alloca)
      throw ("Unknown variable name in for cycle");

  // Emit the start code first, without 'variable' in scope.
  Value *StartVal = Start->codegen();
  if (!StartVal)
    return nullptr;

  // Store the value into the alloca.
  Builder.CreateStore(StartVal, Alloca);

  // Make the new basic block for the loop header, inserting after current
  // block.
  BasicBlock *LoopBB = BasicBlock::Create(TheContext, "loop", TheFunction);

  // Insert an explicit fall through from the current block to the LoopBB.
  Builder.CreateBr(LoopBB);

  // Start insertion in LoopBB.
  Builder.SetInsertPoint(LoopBB);

  // Within the loop, the variable is defined equal to the PHI node.  If it
  // shadows an existing variable, we have to restore it, so save it now.
  Value *OldVal = NamedValues[sugar];
  NamedValues[sugar] = Alloca;

  // Emit the body of the loop.  This, like any other expr, can change the
  // current BB.  Note that we ignore the value computed by the body, but don't
  // allow an error.
  if (!Body->codegen())
    return nullptr;

  // Emit the step value.
  Value *StepVal = nullptr;
  if (Step) {
    StepVal = Step->codegen();
    if (!StepVal)
      return nullptr;
  } else {
    // If not specified, use 1.0.
    StepVal = ConstantInt::get(TheContext, APInt(32, 1, true));
  }

  // Compute the end condition.
  Value *EndCond = Builder.CreateIntCast (
                           Builder.CreateICmpEQ(End->codegen(),
                                                Builder.CreateLoad(Alloca, "endforload"),
                                                "endfor"),
                           Type::getInt32Ty(TheContext),
                           false,
                           "endforcast");
  if (!EndCond)
    return nullptr;

  // Reload, increment, and restore the alloca.  This handles the case where
  // the body of the loop mutates the variable.
  Value *CurVar = Builder.CreateLoad(Alloca, VarName.c_str());
  Value *NextVar = Builder.CreateFAdd(CurVar, StepVal, "nextvar");
  Builder.CreateStore(NextVar, Alloca);

  // Convert condition to a bool by comparing non-equal to 0.0.
  EndCond = Builder.CreateICmpNE(
      EndCond, ConstantInt::get(TheContext, APInt(32, 1, true)), "loopcond");

  // Create the "after loop" block and insert it.
  BasicBlock *AfterBB =
      BasicBlock::Create(TheContext, "afterloop", TheFunction);

  // Insert the conditional branch into the end of LoopEndBB.
  Builder.CreateCondBr(EndCond, LoopBB, AfterBB);

  // Any new code will be inserted in AfterBB.
  Builder.SetInsertPoint(AfterBB);

  // Restore the unshadowed variable.
  if (OldVal)
    NamedValues[sugar] = OldVal;
  else
    NamedValues.erase(sugar);

  // for expr always returns 0.0.
  return Constant::getNullValue(Type::getInt32Ty(TheContext));
}

Value * mila::IfExprAST::codegen() {
  Value *CondV = Cond->codegen();
  if (!CondV)
    return nullptr;

  // Convert condition to a bool by comparing non-equal to 0.0.
  CondV = Builder.CreateICmpNE(
      CondV, ConstantInt::get(TheContext, APInt(32, 0, true)), "ifcond");

  Function *TheFunction = Builder.GetInsertBlock()->getParent();

  // Create blocks for the then and else cases.  Insert the 'then' block at the
  // end of the function.
  BasicBlock *ThenBB = BasicBlock::Create(TheContext, "then", TheFunction);
  BasicBlock *ElseBB = BasicBlock::Create(TheContext, "else");
  BasicBlock *MergeBB = BasicBlock::Create(TheContext, "ifcont");

  Builder.CreateCondBr(CondV, ThenBB, ElseBB);

  // Emit then value.
  Builder.SetInsertPoint(ThenBB);

  Value *ThenV = Then->codegen();
  if (!ThenV)
    return nullptr;

  Builder.CreateBr(MergeBB);
  // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
  ThenBB = Builder.GetInsertBlock();

  // Emit else block.
  TheFunction->getBasicBlockList().push_back(ElseBB);
  Builder.SetInsertPoint(ElseBB);

  Value *ElseV = Else->codegen();
  if (!ElseV)
    return nullptr;

  Builder.CreateBr(MergeBB);
  // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
  ElseBB = Builder.GetInsertBlock();

  // Emit merge block.
  TheFunction->getBasicBlockList().push_back(MergeBB);
  Builder.SetInsertPoint(MergeBB);
  PHINode *PN = Builder.CreatePHI(Type::getInt32Ty(TheContext), 2, "iftmp");

  PN->addIncoming(ThenV, ThenBB);
  PN->addIncoming(ElseV, ElseBB);
  return PN;
}

Value * mila::WhileExprAST::codegen()
{
  Function *TheFunction = Builder.GetInsertBlock()->getParent();

  // Make the new basic block for the loop header, inserting after current
  // block.
  BasicBlock *CondBB = BasicBlock::Create(TheContext, "cond", TheFunction);
  // Create blocks for the body and exit.  Insert the 'cond' block at the
  // end of the function.
  BasicBlock *LoopBB = BasicBlock::Create(TheContext, "loop");
  BasicBlock *ExitBB = BasicBlock::Create(TheContext, "exit");

  // Insert an explicit fall through from the current block to the LoopBB.
  Builder.CreateBr(CondBB);

  // Start insertion in LoopBB.
  Builder.SetInsertPoint(CondBB);
  Value *CondV = Cond->codegen();
  if (!CondV)
    return nullptr;

  // Convert condition to a bool by comparing non-equal to 0.0.
  CondV = Builder.CreateICmpNE(
      CondV, ConstantInt::get(TheContext, APInt(32, 0, true)), "ifcond");


  Builder.CreateCondBr(CondV, LoopBB, ExitBB);

  // Emit then value.
  TheFunction->getBasicBlockList().push_back(LoopBB);
  Builder.SetInsertPoint(LoopBB);

  Value *LoopV = Body->codegen();
  if (!LoopV)
    return nullptr;
  Builder.CreateBr(CondBB);

  TheFunction->getBasicBlockList().push_back(ExitBB);
  Builder.SetInsertPoint(ExitBB);

  return Constant::getNullValue(Type::getInt32Ty(TheContext));
}

//#######################################################################################

Value * mila::VariableExprAST::alloca ( void ) const
{
    std::string sugar = Name + '/' + Builder.GetInsertBlock()->getParent()->getName().str();
    Value * ret = NamedValues [ sugar ];
    if (!ret)
        throw ("Unknown variable name");
    return ret;
}

Value * mila::ArrayExprAST::alloca ( void ) const
{
    std::string sugar = Name + '/' + Builder.GetInsertBlock()->getParent()->getName().str();
    Value * V = NamedValues [ sugar ];
    if (!V)
        throw ("Unknown array name");
    auto ptr = Builder.CreateGEP(Type::getInt32Ty(TheContext), V, Index->codegen());
    return ptr;
}

//#######################################################################################

void mila::ExprListAST::print ( void ) const
{
    std::cerr << "<List>" << std::endl;
    for (const auto &expr : Nodes)
        if (expr)
            expr->print();
        else
            std::cerr << "nullptr" << std::endl;
    std::cerr << "</List>" << std::endl;
}

void mila::NumberExprAST::print ( void ) const
{
    std::cerr << "<Number> " << Val << " </Number>" << std::endl;
}

void mila::ConstExprAST::print ( void ) const
{
    std::cerr << "<Const> " << Name << " = " << Val << " </Const>" << std::endl;
}

void mila::DeclareExprAST::print ( void ) const
{
    std::cerr << "<Declare> " << Name << " [" << -Offset << ".." << Length - Offset << "] </Declare>" << std::endl;
}

void mila::VariableExprAST::print ( void ) const
{
    std::cerr << "<Variable> " << Name << " </Variable>" << std::endl;
}

void mila::ArrayExprAST::print ( void ) const
{
    std::cerr << "<Array> " << Name << " [";
    Index->print();
    std::cerr << "] </Array>" << std::endl;
}

void mila::BinaryExprAST::print ( void ) const
{
    std::cerr << "<Binary>" << std::endl;
    LHS->print();
    std::cerr << Op << std::endl;
    RHS->print();
    std::cerr << "</Binary>" << std::endl;
}

void mila::IfExprAST::print ( void ) const
{
    std::cerr << "<If>" << std::endl;
    std::cerr << "<Condition>" << std::endl;
    Cond->print();
    std::cerr << "</Condition>" << std::endl;
    std::cerr << "<Then>" << std::endl;
    Then->print();
    std::cerr << "</Then>" << std::endl;
    std::cerr << "<Else>" << std::endl;
    if (Else)
        Else->print();
    else
        std::cerr << "nullptr" << std::endl;
    std::cerr << "</Else>" << std::endl;
    std::cerr << "</If>" << std::endl;
}

void mila::ForExprAST::print ( void ) const
{
    std::cerr << "<For>" << std::endl;
    std::cerr << "Start: ";
    Start->print();
    std::cerr << std::endl;
    std::cerr << "End: ";
    End->print();
    std::cerr << std::endl;
    std::cerr << "Step: ";
    Step->print();
    std::cerr << std::endl;
    std::cerr << "Do:" << std::endl;
    Body->print();
    std::cerr << "</For>" << std::endl;
}

void mila::WhileExprAST::print ( void ) const
{
    std::cerr << "<While>" << std::endl;
    std::cerr << "<Condition>" << std::endl;
    Cond->print();
    std::cerr << "</Condition>" << std::endl;
    std::cerr << "<Body>" << std::endl;
    Body->print();
    std::cerr << "</Body>" << std::endl;
    std::cerr << "</While>" << std::endl;
}

void mila::CallExprAST::print ( void ) const
{
    std::cerr << "<Call> " << Callee << std::endl;
    for (const auto &arg : Args)
        arg->print();
    std::cerr << "</Call>" << std::endl;
}

void mila::PrototypeAST::print ( void ) const
{
    std::cerr << "<Prototype> " << Name << std::endl;
    for (const auto &arg : Args)
        std::cerr << arg << std::endl;
    std::cerr << "</Prototype>" << std::endl;
}

void mila::FunctionAST::print ( void ) const
{
    std::cerr << "<Function> " << std::endl;
    Proto->print();
    for (const auto &comm : Body)
        comm->print();
    std::cerr << "</Function>" << std::endl;
}

void mila::LibraryExprAST::print ( void ) const
{
    std::cerr << "<Library> " << Name << ": " << Arg << "</Library>" << std::endl;
}

void mila::ReturnExprAST::print ( void ) const
{
    std::cerr << "<Return></Return>" << std::endl;
}

//#######################################################################################

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for mutable variables etc.
AllocaInst * mila::CreateEntryBlockAlloca(Function *TheFunction,
                                          const std::string &VarName)
{
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
            TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Type::getInt32Ty(TheContext), 0,
            VarName.c_str());
}

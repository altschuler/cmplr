#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/IRBuilder.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/LLVMContext.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/DerivedTypes.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Transforms/Scalar.h"

#include "boost/format.hpp"

#include "AST.hpp"
#include "Errors.hpp"

#ifndef CODEGEN_HPP
#define CODEGEN_HPP

using namespace llvm;

class Codegen {
  map<string, AllocaInst*> NamedValues;
  IRBuilder<> Builder;
  Module *TheModule;
  FunctionPassManager *TheFPM;
  ExecutionEngine *ExecEngine;

public:
  Codegen(ExecutionEngine *execEngine, Module *module);

  Module *GetModule() { return this->TheModule; }
  ExecutionEngine *GetExecEngine() { return this->ExecEngine; }

  AllocaInst *CreateEntryBlockAlloca(Function *func, string varName);

  Value *Generate(ExprAST *expr);
  Value *Generate(NumberExprAST *expr);
  Value *Generate(VariableExprAST *expr);
  Value *Generate(BinaryExprAST *expr);
  Value *Generate(UnaryExprAST *expr);
  Value *Generate(CallExprAST *expr);
  Value *Generate(ConditionalExprAST *expr);
  Value *Generate(ForExprAST *expr);
  Value *Generate(BlockAST *block);

  Function *Generate(OperatorAST *expr);
  Function *Generate(PrototypeAST *proto);
  Function *Generate(FunctionAST *func);

  void CreateArgumentAllocas(PrototypeAST *proto, Function *func);
};

#endif











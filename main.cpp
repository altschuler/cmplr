#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/DerivedTypes.h"
#include "llvm/IRBuilder.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/PassManager.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/ADT/ArrayRef.h"

#include "Parser.hpp"
#include "Errors.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>

using namespace std;
using namespace llvm;

// Top level handling
//-------

static ExecutionEngine *ExecEngine;
static Parser TheParser;

static void HandleDefinition() {
  FunctionAST *func = TheParser.ParseDefinition();
  Function *code = func->Codegen();
  if (func && code) 
  {
	//	  fprintf(stderr, "Parsed definition.\n");
	//	  code->dump();
  }
  else
	TheParser.GetNextToken();
}

static void HandleExtern() {
  PrototypeAST *ext = TheParser.ParseExtern();
  Function *code = ext->Codegen();
  if (ext && code) 
  {
	//	  fprintf(stderr, "Parsed an extern.\n");
	//	  code->dump();
  }
  else
	TheParser.GetNextToken();
}

static void HandleTopLevelExpr() {
  FunctionAST *expr = TheParser.ParseTopLevelExpr();
  Function *code = expr->Codegen();
  if (expr && code)
  {
	  void *funcPtr = ExecEngine->getPointerToFunction(code);

	  double (*fptr)() = (double (*)())(intptr_t)funcPtr;
	  fprintf(stderr, "[%i] %f\n", code->getValueID(), fptr());
	  //	  code->dump();
  }
  else
	TheParser.GetNextToken();
}

static void MainLoop() {
  while(1) {
	fprintf(stderr, ">");
	switch(TheParser.GetCurTok()) {
	case tok_eof: return;
	case tok_def: HandleDefinition(); break;
	case tok_extern: HandleExtern(); break;
	case ';': TheParser.GetNextToken(); break; // ignore ;
	default: HandleTopLevelExpr(); break;
	}
  }
}


// Code generation
//----------------

static Module *TheModule;
static FunctionPassManager *TheFPM;
static IRBuilder<> Builder(getGlobalContext());
static map<string, Value*> NamedValues;

// IR code
Value *NumberExprAST::Codegen() {
  return ConstantFP::get(getGlobalContext(), APFloat(Val));
}

Value *VariableExprAST::Codegen() {
  Value *val = NamedValues[Name];
  return val ? val : ErrorV("Unknown variable name");
}

Value *BinaryExprAST::Codegen() {
  Value *L = LHS->Codegen();
  Value *R = RHS->Codegen();
  if (L == 0 || R == 0)
	return 0;

  switch (Op) {
  case '+': return Builder.CreateFAdd(L, R, "addtmp");
  case '-': return Builder.CreateFSub(L, R, "subtmp");
  case '*': return Builder.CreateFMul(L, R, "multmp");
  case '<': 
	L = Builder.CreateFCmpULT(L, R, "cmptmp");
	return Builder.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()), "booltmp");

  default: return ErrorV("Unknown operator");
  }
}

Value *CallExprAST::Codegen() {
  Function *CalleeF = TheModule->getFunction(Callee);
  if (CalleeF == 0)
	return ErrorV("Unknown function name");

  if (CalleeF->arg_size() != Args.size())
	return ErrorV("Wrong number of arguments");

  vector<Value*> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
	ArgsV.push_back(Args[i]->Codegen());
	if (ArgsV.back() == 0)
	  return 0;
  }

  ArrayRef<Value*> *argArr = new ArrayRef<Value*>(ArgsV);
  return Builder.CreateCall(CalleeF, *argArr, "tmpcall");
}

Value *ConditionalExprAST::Codegen() {
  Value *condVal = Cond->Codegen();
  if (condVal == 0)
	return 0;

  condVal = Builder.CreateFCmpONE(condVal, ConstantFP::get(getGlobalContext(), APFloat(0.0)), "ifcond");

  Function *func = Builder.GetInsertBlock()->getParent();

  BasicBlock *thenBlock = BasicBlock::Create(getGlobalContext(), "then", func);
  BasicBlock *elseBlock = BasicBlock::Create(getGlobalContext(), "else");
  BasicBlock *mergeBlock = BasicBlock::Create(getGlobalContext(), "merge");

  Builder.CreateCondBr(condVal, thenBlock, elseBlock);

  Builder.SetInsertPoint(thenBlock);
  
  Value *thenVal = Then->Codegen();
  if (thenVal == 0)
	return 0;

  Builder.CreateBr(mergeBlock);

  thenBlock = Builder.GetInsertBlock();
  
  func->getBasicBlockList().push_back(elseBlock);
  Builder.SetInsertPoint(elseBlock);
  
  Value *elseVal = Else->Codegen();
  if (elseVal == 0)
	return 0;

  Builder.CreateBr(mergeBlock);
  elseBlock = Builder.GetInsertBlock();

  func->getBasicBlockList().push_back(mergeBlock);
  Builder.SetInsertPoint(mergeBlock);

  PHINode *phi = Builder.CreatePHI(Type::getDoubleTy(getGlobalContext()), 2, "iftmp");
  phi->addIncoming(thenVal, thenBlock);
  phi->addIncoming(elseVal, elseBlock);

  return phi;
}

Function *PrototypeAST::Codegen() 
{
  vector<Type*> Doubles(Args.size(), Type::getDoubleTy(getGlobalContext()));
  FunctionType *FT = FunctionType::get(Type::getDoubleTy(getGlobalContext()), Doubles, false);
  Function* F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule);
  

  if (F->getName() != Name) 
  {
	F->eraseFromParent();
	F = TheModule->getFunction(Name);
	
	if (!F->empty()) 
	{
	  ErrorF("Redefinition of function");
	  return 0;
	}
	
	if (F->arg_size() != Args.size()) 
	{
	  Error("Redefinition of function with wrong number of arguments");
	  return 0;
	}
  }
  
  unsigned Idx = 0;
  for (Function::arg_iterator AI = F->arg_begin(); Idx != Args.size(); ++AI, ++Idx)
  {
	  AI->setName(Args[Idx]);
	  NamedValues[Args[Idx]] = AI;
  }

  return F;
}

Function *FunctionAST::Codegen() 
{
	NamedValues.clear();
	
	Function *func = Prototype->Codegen();
	if (func == 0)
		return 0;
	
	BasicBlock *block = BasicBlock::Create(getGlobalContext(), "entry", func);
	Builder.SetInsertPoint(block);
	
	Value *retVal = Body->Codegen();
	if (retVal) 
	{
		Builder.CreateRet(retVal);
		verifyFunction(*func);
		TheFPM->run(*func);
		return func;
	}

	func->eraseFromParent();
	return 0;
}

int main() 
{
	InitializeNativeTarget();
	LLVMContext &Context = getGlobalContext();

	TheModule = new Module("JIT", Context);

	string ErrStr;
	ExecEngine = EngineBuilder(TheModule).setErrorStr(&ErrStr).create();
	if (!ExecEngine)
	{
	  fprintf(stderr, "Could not create ExecutionEngine: %s\n", ErrStr.c_str());
	  exit(1);
	}

	// Set up function optimization
	FunctionPassManager FPM(TheModule);
	FPM.add(new DataLayout(*ExecEngine->getDataLayout()));
	FPM.add(createBasicAliasAnalysisPass());
	FPM.add(createInstructionCombiningPass());
	FPM.add(createReassociatePass());
	FPM.add(createGVNPass());
	FPM.add(createCFGSimplificationPass());
	FPM.doInitialization();

	TheFPM = &FPM;

	fprintf(stderr, ">");
	TheParser.GetNextToken();
	  
	MainLoop();

	TheFPM = 0;
	TheModule->dump();
	
	return 0;
}

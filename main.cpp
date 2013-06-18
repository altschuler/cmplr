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

#include "Lexer.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>

using namespace std;
using namespace llvm;

// AST structures
//---------------
class ExprAST {
public:
  virtual ~ExprAST() {};
  virtual Value *Codegen() = 0;
};

// Numeric literals
class NumberExprAST : public ExprAST {
  double Val;
public:
  NumberExprAST(double val) : Val(val) {}
  virtual Value *Codegen();
};

// Variable
class VariableExprAST : public ExprAST {
  string Name;
public:
  VariableExprAST(const string &name) : Name(name) {}
  virtual Value *Codegen();
};

// Binary op
class BinaryExprAST : public ExprAST {
  char Op;
  ExprAST *LHS, *RHS;
public:
  BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs) : Op(op), LHS(lhs), RHS(rhs) {} 
  virtual Value *Codegen();
};

// Function call
class CallExprAST : public ExprAST {
  string Callee;
  vector<ExprAST*> Args;
public:
  CallExprAST(const string &callee, vector<ExprAST*> &args) : Callee(callee), Args(args) {}
  virtual Value *Codegen();
};

// Function signature definition
class PrototypeAST {
  string Name;
  vector<string> Args;
public:
  PrototypeAST(const string &name, vector<string> &args) : Name(name), Args(args) {}
  virtual Function *Codegen();
};

// Function definition
class FunctionAST {
  PrototypeAST *Prototype;
  ExprAST *Body;
public:
  FunctionAST(PrototypeAST *prototype, ExprAST *body) : Prototype(prototype), Body(body) {}
  virtual Function *Codegen();
};

// Parser
//-------
static Lexer TheLexer;

static int CurTok;
static int getNextToken() {
  return CurTok = TheLexer.GetToken();
}

// Error helpers
ExprAST *Error(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return 0;
}

PrototypeAST *ErrorP(const char *Str) {
  Error(Str);
  return 0;
}

FunctionAST *ErrorF(const char *Str) {
  Error(Str);
  return 0;
}

// Expression parsing
static ExprAST *ParseNumberExpr() {
  ExprAST *Result = new NumberExprAST(TheLexer.GetNumVal());
  getNextToken();
  return Result;
}

static ExprAST *ParseExpression();

static ExprAST *ParseParenExpr() {
  getNextToken(); // eat (
  ExprAST *V = ParseExpression();
  if (!V) 
	return 0;

  if (CurTok != ')')
	return Error("Expected ')'");

  getNextToken(); // eat )

  return V;
}

static ExprAST *ParseIdentifierExpr() {
	string IdName = TheLexer.GetIdentifierStr();
  
  getNextToken(); // eat the identifier

  if (CurTok != '(')
	return new VariableExprAST(IdName);

  getNextToken();
  vector<ExprAST*> Args;
  if (CurTok != ')') {
	while (1) {
	  ExprAST *Arg = ParseExpression();
	  if (!Arg) 
		return 0;

	  Args.push_back(Arg);
	  if (CurTok == ')')
		break;

//	  if (CurTok != ',')
//		return Error("Expected ')' or ',' in argument list");
//	  getNextToken(); // eat ,
	}
  }

  getNextToken(); // eat )
  
  return new CallExprAST(IdName, Args);
}

static ExprAST *ParsePrimary() {
  switch (CurTok) {
  case tok_identifier: return ParseIdentifierExpr();
  case tok_number: return ParseNumberExpr();
  case '(': return ParseParenExpr();
  default: return Error("Expected expression");
  }
}

static map<char, int> BinopPrecedence;

static int GetTokPrecedence() {
  if (!isascii(CurTok))
	return -1;

  int prec = BinopPrecedence[CurTok];
  if (prec <= 0)
	return -1;

  return prec;
}

static ExprAST* ParseBinOpRHS(int exprPrec, ExprAST* lhs);

static ExprAST *ParseExpression() {
  ExprAST *LHS = ParsePrimary();
  if (!LHS)
	return 0;

  return ParseBinOpRHS(0, LHS);
}

static ExprAST* ParseBinOpRHS(int exprPrec, ExprAST* lhs) {
  while(1) {
	int tokPrec = GetTokPrecedence();

	if (tokPrec < exprPrec)
	  return lhs;

	int binOp = CurTok;
	getNextToken(); // eat the operator
	
	ExprAST *rhs = ParsePrimary();
	if (!rhs)
	  return 0;

	int nextTokPrec = GetTokPrecedence();
	if (tokPrec < nextTokPrec) {
	  rhs = ParseBinOpRHS(tokPrec + 1, rhs);
	  if (rhs == 0)
		return 0;
	}

	lhs = new BinaryExprAST(binOp, lhs, rhs);
  }
}

static PrototypeAST *ParsePrototype() {
  if (CurTok != tok_identifier)
	return ErrorP("Expected name of function");

  string name = TheLexer.GetIdentifierStr();
  getNextToken(); // eat name
  
  if (CurTok != '(')
	return ErrorP("Expected '(' after name of function");

  vector<string> args;
  while (getNextToken() == tok_identifier) {
	  args.push_back(TheLexer.GetIdentifierStr());
  }

  if (CurTok != ')')
	return ErrorP("Expected ')' at end of function name");

  getNextToken(); // eat )

  return new PrototypeAST(name, args);  
}

static FunctionAST *ParseDefinition() {
  getNextToken(); // eat "def"
  PrototypeAST *prototype = ParsePrototype();
  if (prototype == 0)
	return 0;

  ExprAST *body = ParseExpression();
  if (body != 0)
	return new FunctionAST(prototype, body);
  
  return 0;  
}

static PrototypeAST *ParseExtern() {
  getNextToken(); // eat "extern"
  return ParsePrototype();
}

// Top level expressions
static FunctionAST *ParseTopLevelExpr() {
  if (ExprAST *expr = ParseExpression()) {
	vector<string> args;
	PrototypeAST *prototype = new PrototypeAST("", args);
	return new FunctionAST(prototype, expr);
  }
  return 0;
}

// Top level handling
//-------

static ExecutionEngine *ExecEngine;

static void HandleDefinition() {
  FunctionAST *func = ParseDefinition();
  Function *code = func->Codegen();
  if (func && code) 
  {
	  fprintf(stderr, "Parsed definition.\n");
	  code->viewCFG();
//code->getReturnType()->getStructName().str()
//	  code->dump();
  }
  else
	getNextToken();
}

static void HandleExtern() {
  PrototypeAST *ext = ParseExtern();
  Function *code = ext->Codegen();
  if (ext && code) 
  {
	  fprintf(stderr, "Parsed an extern.\n");
//	  code->dump();
  }
  else
	getNextToken();
}

static void HandleTopLevelExpr() {
  FunctionAST *expr = ParseTopLevelExpr();
  Function *code = expr->Codegen();
  if (expr && code)
  {
	  void *funcPtr = ExecEngine->getPointerToFunction(code);

	  double (*fptr)() = (double (*)())(intptr_t)funcPtr;
	  fprintf(stderr, "[%i] %f\n", code->getValueID(), fptr());
//	  fprintf(stderr, "Parsed top level expression\n");	  
//	  code->dump();
  }
  else
	getNextToken();
}

static void MainLoop() {
  while(1) {
	fprintf(stderr, ">");
	switch(CurTok) {
	case tok_eof: return;
	case tok_def: HandleDefinition(); break;
	case tok_extern: HandleExtern(); break;
	case ';': getNextToken(); break; // ignore ;
	default: HandleTopLevelExpr(); break;
	}
  }
}


// Code generation
//----------------
Value *ErrorV(const char *Str) {
  Error(Str);
  return 0;
}

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
	
	if (Value *retVal = Body->Codegen()) 
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

	BinopPrecedence['<'] = 10;
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['*'] = 40;

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
	getNextToken();
	  
	MainLoop();

	TheFPM = 0;
	TheModule->dump();
	
	return 0;
}

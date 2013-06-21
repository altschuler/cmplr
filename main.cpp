#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"

#include "boost/format.hpp"

#include "Parser.hpp"
#include "Errors.hpp"
//#include "BuiltIn.hpp"
#include "Codegen.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>

using namespace std;
using namespace llvm;

// Top level handling
//-------

static Parser TheParser;
static Codegen *Gen;

static void HandleDefinition() {
  FunctionAST *func = TheParser.ParseDefinition();
  Function *code = Gen->Generate(func);
  if (func && code) 
	{
	  fprintf(stderr, "Function '%s' defined.\n", func->GetPrototype()->GetName().c_str());
	}
  else
	TheParser.GetNextToken();
}

static void HandleExtern() {
  PrototypeAST *ext = TheParser.ParseExtern();
  Function *code = Gen->Generate(ext);
  if (ext && code) 
	{
	  fprintf(stderr, "Extern '%s' defined.\n", ext->GetName().c_str());
	}
  else
	TheParser.GetNextToken();
}

static void HandleTopLevelExpr() {
  FunctionAST *expr = TheParser.ParseTopLevelExpr();
  Function *code = Gen->Generate(expr);
  if (expr && code) {
	void *funcPtr = Gen->GetExecEngine()->getPointerToFunction(code);

	double (*fptr)() = (double (*)())(intptr_t)funcPtr;
	fprintf(stderr, "%f\n", fptr());
  }
  else
	TheParser.GetNextToken();
}

static void MainLoop() {
  while(1) {
	switch(TheParser.GetCurTok()) {
	case tok_eof: return;
	case tok_def: 	  
	  fprintf(stderr, "= ");
	  HandleDefinition(); 
	  break;
	case tok_extern: 
	  fprintf(stderr, "= ");
	  HandleExtern(); 
	  break;
	case ';': 
	  fprintf(stderr, ">");
	  TheParser.GetNextToken(); 
	  break; // eat ;
	default: 
	  fprintf(stderr, "= ");
	  HandleTopLevelExpr(); 
	  break;
	}
  }
}

static void CreateBuiltIn(string name, string arg) {
  vector<string> args;
  args.push_back(arg);
  PrototypeAST *proto = new PrototypeAST(name, args);
  Gen->Generate(proto);
}

static void CreateBuiltIn(string name) {
  vector<string> args;
  PrototypeAST *proto = new PrototypeAST(name, args);
  Gen->Generate(proto);
}

void InitBuiltIns() {
  CreateBuiltIn("sin", "a");
  CreateBuiltIn("cos", "a");
  CreateBuiltIn("exit");
}

int main() {
  InitializeNativeTarget();

  LLVMContext &Context = getGlobalContext();
  Module *module = new Module("JIT", Context);

  string ErrStr;
  ExecutionEngine *execEngine = EngineBuilder(module).setErrorStr(&ErrStr).create();
  if (!execEngine) {
	fprintf(stderr, "Could not create ExecutionEngine: %s\n", ErrStr.c_str());
	exit(1);
  }

  Gen = new Codegen(execEngine, module);

  InitBuiltIns();

  fprintf(stderr, ">");
  TheParser.GetNextToken();
	  
  MainLoop();

  return 0;
}

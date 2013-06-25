#include "Driver.hpp"

void Driver::HandleDefinition() {
  FunctionAST *func = TheParser.ParseDefinition();
  Function *code = Gen->Generate(func);
  if (func && code) {
	//	fprintf(stderr, "Function '%s' defined.\n", func->GetPrototype()->GetName().c_str());
  }
  else
	TheParser.GetNextToken();
}

void Driver::HandleOperator() {
  OperatorAST *func = TheParser.ParseOperator();
  Function *code = Gen->Generate(func);
  if (func && code) {
	//	  fprintf(stderr, "Operator '%c' defined.\n", func->GetOp());
  }
  else
	TheParser.GetNextToken();
}

void Driver::HandleExtern() {
  PrototypeAST *ext = TheParser.ParseExtern();
  Function *code = Gen->Generate(ext);
  if (ext && code) {
	//	fprintf(stderr, "Extern '%s' defined.\n", ext->GetName().c_str());
  }
  else
	TheParser.GetNextToken();
}

void Driver::HandleTopLevelExpr() {
  FunctionAST *expr = TheParser.ParseTopLevelExpr();
  Function *code = Gen->Generate(expr);
  if (expr && code) {
	void *funcPtr = Gen->GetExecEngine()->getPointerToFunction(code);

	double (*fptr)() = (double (*)())(intptr_t)funcPtr;
	fptr();
  }
  else
	TheParser.GetNextToken();
}

void Driver::Go(string file) {
  TheParser.SetInputFile(file);

  TheParser.GetNextToken();

  while(1) {
	switch(TheParser.GetCurTok()) {
	case tok_eof: return;
	case tok_def: 	  
	  //	  fprintf(stderr, "= ");
	  HandleDefinition(); 
	  break;
	case tok_extern: 
	  //	  fprintf(stderr, "= ");
	  HandleExtern(); 
	  break;
	case tok_op: 
	  //	  fprintf(stderr, "= ");
	  HandleOperator(); 
	  break;
	case ';': 
	  //	  fprintf(stderr, ">");
	  TheParser.GetNextToken(); 
	  break; // eat ;
	default: 
	  //	  fprintf(stderr, "= ");
	  HandleTopLevelExpr(); 
	  break;
	}
  }
}

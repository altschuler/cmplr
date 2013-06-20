#include "llvm/DerivedTypes.h"
#include <cstdio>
#include <cstdlib>

#include "Parser.hpp"
#include "Errors.hpp"

#ifndef PARSER_CPP
#define PARSER_CPP

using namespace std;
using namespace llvm;

Parser::Parser() {
	BinopPrecedence['<'] = 10;
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['*'] = 40;
}

int Parser::GetNextToken() {
  return CurTok = TheLexer.GetToken();
}

int Parser::GetCurTok() {
  return CurTok;
}

// Expression parsing
ExprAST *Parser::ParseNumberExpr() {
  ExprAST *Result = new NumberExprAST(TheLexer.GetNumVal());
  this->GetNextToken();
  return Result;
}

ExprAST *Parser::ParseParenExpr() {
  this->GetNextToken(); // eat (
  ExprAST *V = ParseExpression();
  if (!V) 
	return 0;

  if (CurTok != ')')
	return Error("Expected ')'");

  this->GetNextToken(); // eat )

  return V;
}

ExprAST *Parser::ParseIdentifierExpr() {
  string IdName = TheLexer.GetIdentifierStr();
  
  this->GetNextToken(); // eat the identifier

  if (CurTok != '(')
	return new VariableExprAST(IdName);

  this->GetNextToken();
  vector<ExprAST*> Args;
  if (CurTok != ')') {
	while (1) {
	  ExprAST *Arg = this->ParseExpression();
	  if (!Arg) 
		return 0;

	  Args.push_back(Arg);
	  if (CurTok == ')')
		break;
	}
  }

  this->GetNextToken(); // eat )
  
  return new CallExprAST(IdName, Args);
}

ExprAST *Parser::ParsePrimary() {
  switch (CurTok) {
  case tok_identifier: return this->ParseIdentifierExpr();
  case tok_number: return this->ParseNumberExpr();
  case tok_if: return this->ParseConditional();
  case '(': return this->ParseParenExpr();
  default: return Error("Expected expression");
  }
}

ExprAST *Parser::ParseConditional() {
  this->GetNextToken(); // eat if
  
  ExprAST *cond = this->ParseExpression();

  if (this->GetCurTok() != tok_then)
	return Error("Expected 'then'");

  this->GetNextToken(); // eat then

  ExprAST *then = this->ParseExpression();

  if (this->GetCurTok() != tok_else)
	return Error("Expected 'else'");
  
  this->GetNextToken(); // eat else

  ExprAST *els = this->ParseExpression();  

  return new ConditionalExprAST(cond, then, els);
}

int Parser::GetTokPrecedence() {
  if (!isascii(CurTok))
	return -1;

  int prec = BinopPrecedence[CurTok];
  if (prec <= 0)
	return -1;

  return prec;
}

ExprAST *Parser::ParseExpression() {
  ExprAST *LHS = this->ParsePrimary();
  if (!LHS)
	return 0;

  return this->ParseBinOpRHS(0, LHS);
}

ExprAST *Parser::ParseBinOpRHS(int exprPrec, ExprAST* lhs) {
  while(1) {
	int tokPrec = this->GetTokPrecedence();

	if (tokPrec < exprPrec)
	  return lhs;

	int binOp = CurTok;
	this->GetNextToken(); // eat the operator
	
	ExprAST *rhs = this->ParsePrimary();
	if (!rhs)
	  return 0;

	int nextTokPrec = this->GetTokPrecedence();
	if (tokPrec < nextTokPrec) {
	  rhs = this->ParseBinOpRHS(tokPrec + 1, rhs);
	  if (rhs == 0)
		return 0;
	}

	lhs = new BinaryExprAST(binOp, lhs, rhs);
  }
}

PrototypeAST *Parser::ParsePrototype() {
  if (CurTok != tok_identifier)
	return ErrorP("Expected name of function");

  string name = TheLexer.GetIdentifierStr();
  this->GetNextToken(); // eat name
  
  if (CurTok != '(')
	return ErrorP("Expected '(' after name of function");

  vector<string> args;
  while (this->GetNextToken() == tok_identifier) {
	args.push_back(TheLexer.GetIdentifierStr());
  }

  if (CurTok != ')')
	return ErrorP("Expected ')' at end of function name");

  this->GetNextToken(); // eat )

  return new PrototypeAST(name, args);  
}

FunctionAST *Parser::ParseDefinition() {
  this->GetNextToken(); // eat "def"
  PrototypeAST *prototype = this->ParsePrototype();
  if (prototype == 0)
	return 0;

  ExprAST *body = this->ParseExpression();
  if (body != 0)
	return new FunctionAST(prototype, body);
  
  return 0;  
}

PrototypeAST *Parser::ParseExtern() {
  this->GetNextToken(); // eat "extern"
  return this->ParsePrototype();
}

  // Top level expressions
FunctionAST *Parser::ParseTopLevelExpr() {
  if (ExprAST *expr = this->ParseExpression()) {
	vector<string> args;
	PrototypeAST *prototype = new PrototypeAST("", args);
	return new FunctionAST(prototype, expr);
  }
  return 0;
}
#endif

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

int Parser::GetTokPrecedence() {
  if (!isascii(CurTok))
	return -1;

  int prec = BinopPrecedence[CurTok];
  if (prec <= 0)
	return -1;

  return prec;
}


//--------------------
// Expression parsing
//--------------------

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
  case tok_for: return this->ParseFor();
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

ExprAST *Parser::ParseFor() {
  // eat 'for'
  this->GetNextToken();

  string iterName = TheLexer.GetIdentifierStr();
  
  this->GetNextToken();
  if (CurTok != '=')
	return Error("For loop initializor must be assignment, missing '='");

  // eat '='
  this->GetNextToken();

  ExprAST *init = this->ParseExpression();
  if (init == 0)
	return 0;

  if (CurTok != ',')
	return Error("Expected comma after for loop initializor");
  
  // eat ','
  this->GetNextToken();

  ExprAST *end = this->ParseExpression();
  if (end == 0)
	return 0;
  
  ExprAST *step = 0;
  if (CurTok == ',') {
	// eat ','
	this->GetNextToken();
	
	step = this->ParseExpression();
	if (step == 0)
	  return 0;
  }

  if (CurTok != tok_in)
	return Error("Expected 'in' before loop body");

  // eat 'in'
  this->GetNextToken();

  ExprAST *body = this->ParseExpression();

  return new ForExprAST(iterName, init, step, end, body);
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
	
	// if expression has higher precedence, return lhs
	// eg if next token is not an operator
	if (tokPrec < exprPrec)
	  return lhs;
	
	// save the current operator
	int binOp = CurTok;

	// eat the current operator and parse the rhs
	this->GetNextToken(); // eat the operator	
	ExprAST *rhs = this->ParsePrimary();
	if (!rhs)
	  return 0;
	
	// CurTok is now expected to be either an operator or
	// not, if not precedence is -1 (and thus always <= tokPrec)
	//
	// if the next token has higher precedence than the current
	// then 
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
  this->GetNextToken(); // eat 'def'
  PrototypeAST *prototype = this->ParsePrototype();
  if (prototype == 0)
	return 0;

  ExprAST *body = this->ParseExpression();
  if (body != 0)
	return new FunctionAST(prototype, body);
  
  return 0;  
}

OperatorAST *Parser::ParseOperator() {
  // eat 'op'
  this->GetNextToken(); 
  
  // save the operator and precedence
  char op = CurTok;
  // eat operator
  this->GetNextToken();

  if (CurTok != tok_number)
	return ErrorO("Expected precedence after operator");

  int prec = TheLexer.GetNumVal();
  // eat precendence
  this->GetNextToken();

  if (CurTok != '(') 
	return ErrorO("Expected '(' after operator precedence");

  // eat '('
  this->GetNextToken();

  vector<string> args;
  while (CurTok == tok_identifier) {
	args.push_back(TheLexer.GetIdentifierStr());
	this->GetNextToken();
  }

  if (args.size() > 2 || args.size() < 1) {
	return ErrorO("Wrong number of arguments for operator definition, 1 or 2 expected");
  }

  if (CurTok != ')') 
	return ErrorO("Expected ')' after operator arguments");

  // eat ')'
  this->GetNextToken();

  ExprAST* body = this->ParseExpression();

  // install precedence
  BinopPrecedence[op] = prec;

  return new OperatorAST(op, prec, args, body);
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

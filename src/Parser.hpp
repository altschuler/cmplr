#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>

#include "Lexer.hpp"
#include "AST.hpp"

#ifndef PARSER_HPP
#define PARSER_HPP

class Parser {
  map<char, int> BinopPrecedence;
  Lexer TheLexer;
  int CurTok;

public:
  Parser();
  int GetCurTok();
  int GetNextToken();

  void SetInputFile(string file, int initialSeek);

  ExprAST *ParseNumberExpr();
  ExprAST *ParseExpression();
  ExprAST *ParseParenExpr();
  ExprAST *ParseIdentifierExpr();
  ExprAST *ParseUnary();
  ExprAST *ParsePrimary();
  ExprAST *ParseBinOpRHS(int exprPrec, ExprAST* lhs);
  ExprAST *ParseConditional();
  ExprAST *ParseFor();

  PrototypeAST *ParsePrototype();
  FunctionAST *ParseDefinition();
  OperatorAST *ParseOperator();
  PrototypeAST *ParseExtern();
  FunctionAST *ParseTopLevelExpr();

  ImportAST *ParseImport();
  
private:
  int GetTokPrecedence();
};
#endif

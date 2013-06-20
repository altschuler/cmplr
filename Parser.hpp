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
  ExprAST *ParseNumberExpr();
  ExprAST *ParseExpression();
  ExprAST *ParseParenExpr();
  ExprAST *ParseIdentifierExpr();
  ExprAST *ParsePrimary();
  ExprAST *ParseBinOpRHS(int exprPrec, ExprAST* lhs);
  ExprAST *ParseConditional();
  PrototypeAST *ParsePrototype();
  FunctionAST *ParseDefinition();
  PrototypeAST *ParseExtern();
  FunctionAST *ParseTopLevelExpr();


private:
  int GetTokPrecedence();
};
#endif

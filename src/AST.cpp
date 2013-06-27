#include "AST.hpp"

BlockAST::BlockAST(ExprAST *expr) {
  this->AppendExpression(expr);
}

void BlockAST::AppendExpression(ExprAST *expr) {
  Expressions.push_back(expr);
}





















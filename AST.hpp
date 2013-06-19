// AST structures
//---------------
#include "llvm/DerivedTypes.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>

#ifndef AST_HPP
#define AST_HPP

using namespace std;
using namespace llvm;

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
#endif

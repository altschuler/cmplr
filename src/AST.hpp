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

enum ASTType {
  ASTNumberExpr, ASTVariableExpr, ASTBinaryExpr, ASTCallExpr, 
  ASTConditionalExpr, ASTForExpr,
  ASTPrototype, ASTFunction,
  ASTOperator, ASTUnary,
  ASTBlock,
};

struct ImportAST {
  string FileName;
  int ParentCursorPosition;

public:
  ImportAST(string fileName, int pcp) : FileName(fileName), ParentCursorPosition(pcp) {}
};

class ExprAST {
public:
  virtual ~ExprAST() {};
  virtual ASTType GetASTType() = 0;
};

// list of expressions form a block
class BlockAST {
  vector<ExprAST*> Expressions;

public:
  // TODO kill this constructor
  BlockAST(int test) {}
  BlockAST(ExprAST *expr);

  void AppendExpression(ExprAST *expr);
  
  vector<ExprAST*> GetExpressions() { return this->Expressions; }

  ASTType GetASTType() { return ASTBlock; }
};

// Numeric literals
class NumberExprAST : public ExprAST {
  double Val;
public:
  NumberExprAST(double val) : Val(val) {}
  double GetVal() { return this->Val; }

  virtual ASTType GetASTType() { return ASTNumberExpr; }
};

// Variable
class VariableExprAST : public ExprAST {
  string Name;
public:
  VariableExprAST(const string &name) : Name(name) {}
  string GetName() { return this->Name; }

  virtual ASTType GetASTType() { return ASTVariableExpr; }
};

// Binary op
class BinaryExprAST : public ExprAST {
  char Op;
  ExprAST *LHS, *RHS;
public:
  BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs) : Op(op), LHS(lhs), RHS(rhs) {} 
  char GetOp() { return this->Op; }
  ExprAST *GetLHS() { return this->LHS; }
  ExprAST *GetRHS() { return this->RHS; }

  virtual ASTType GetASTType() { return ASTBinaryExpr; }
};

class ConditionalExprAST : public ExprAST {
  ExprAST *Cond, *Then, *Else;
public:
  ConditionalExprAST(ExprAST *cond, ExprAST *then, ExprAST *els) 
	: Cond(cond), Then(then), Else(els) {}
  
  ExprAST *GetCond() { return this->Cond; }
  ExprAST *GetThen() { return this->Then; }
  ExprAST *GetElse() { return this->Else; }

  virtual ASTType GetASTType() { return ASTConditionalExpr; }
};

class ForExprAST : public ExprAST {
  string IterName;
  ExprAST *Init, *Step, *End;
  BlockAST *Body;
public:
  ForExprAST(string iterName, ExprAST *init, ExprAST *step, ExprAST *end, BlockAST *body)
	: IterName(iterName), Init(init), Step(step), End(end), Body(body) {}

  string GetIterName() { return this->IterName; }
  ExprAST *GetInit() { return this->Init; }
  ExprAST *GetStep() { return this->Step; }
  ExprAST *GetEnd() { return this->End; }
  BlockAST *GetBody() { return this->Body; }

  virtual ASTType GetASTType() { return ASTForExpr; }  
};

// Function call
class CallExprAST : public ExprAST {
  string Callee;
  vector<ExprAST*> Args;
public:
  CallExprAST(const string &callee, vector<ExprAST*> &args) : Callee(callee), Args(args) {}

  string GetCallee() { return this->Callee; }
  vector<ExprAST*> GetArgs() { return this->Args; }

  virtual ASTType GetASTType() { return ASTCallExpr; }
};

// Function signature definition
class PrototypeAST {
  string Name;
  vector<string> Args;
public:
  PrototypeAST(const string &name, vector<string> &args) : Name(name), Args(args) {}

  string GetName() { return this->Name; };
  vector<string> GetArgs() { return this->Args; };

  ASTType GetASTType() { return ASTPrototype; }
};

// Function definition
class FunctionAST {
  PrototypeAST *Prototype;
  BlockAST *Body;
public:
  FunctionAST(PrototypeAST *prototype, BlockAST *body) : Prototype(prototype), Body(body) {}

  PrototypeAST *GetPrototype() { return this->Prototype; };
  BlockAST *GetBody() { return this->Body; };

  ASTType GetASTType() { return ASTFunction; }
};

class UnaryExprAST : public ExprAST {
  char Op;
  ExprAST *Operand;
public:
  UnaryExprAST(char op, ExprAST *operand) : Op(op), Operand(operand) {}

  char GetOp() { return this->Op; }
  ExprAST *GetOperand() { return this->Operand; }

  ASTType GetASTType() { return ASTUnary; }
};

// Operator definition
class OperatorAST {
  char Op;
  int Precedence;
  vector<string> Args;
  BlockAST *Body;
public:
  OperatorAST(char op, int prec, vector<string> args, BlockAST *body) 
	: Op(op), Precedence(prec), Args(args), Body(body) {}

  char GetOp() { return this->Op; };
  int GetPrecedence() { return this->Precedence; };
  vector<string> GetArgs() { return this->Args; };
  BlockAST *GetBody() { return this->Body; };
  
  bool IsBinary() { return this->Args.size() == 2; };

  ASTType GetASTType() { return ASTOperator; }
};

#endif

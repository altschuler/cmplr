#include "llvm/Value.h"
#include "AST.hpp"

#ifndef ERRORS_HPP
#define ERRORS_HPP

// Error helpers
ExprAST *Error(const char *Str);
PrototypeAST *ErrorP(const char *Str);
FunctionAST *ErrorF(const char *Str);
OperatorAST *ErrorO(const char *Str);
Value *ErrorV(const char *Str);
ImportAST *ErrorI(const char *Str);
const char *formatErr(string msg, string arg);

#endif











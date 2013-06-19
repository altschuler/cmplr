#include "llvm/Value.h"
#include "AST.hpp"

#ifndef ERRORS_CPP
#define ERRORS_CPP

// Error helpers
ExprAST *Error(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return 0;
}

PrototypeAST *ErrorP(const char *Str) {
  Error(Str);
  return 0;
}

FunctionAST *ErrorF(const char *Str) {
  Error(Str);
  return 0;
}

Value *ErrorV(const char *Str) {
  Error(Str);
  return 0;
}
#endif

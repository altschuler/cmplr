#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/LLVMContext.h"
#include "llvm/Module.h"

#include "boost/format.hpp"

#include "Parser.hpp"
#include "Errors.hpp"
#include "Codegen.hpp"

#include <iostream>

#ifndef DRIVER_HPP
#define DRIVER_HPP

using namespace std;
using namespace llvm;

class Driver {
  Parser TheParser;
  Codegen *Gen;
  string CurrentFile;

public:
  Driver(Codegen *codegen) : Gen(codegen) {}
  void Go(string file);

private:
  void HandleDefinition();
  void HandleOperator();
  void HandleExtern();
  void HandleTopLevelExpr();
  void HandleImport();
};

#endif

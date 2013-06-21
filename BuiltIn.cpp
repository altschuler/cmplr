#include "BuiltIn.hpp"

#ifndef BUILT_INT_HPP
#define BUILT_INT_HPP

using namespace std;
using namespace llvm;

static Codegen* Gen;

static void CreateBuiltIn(string name, string arg) {
  vector<string> args;
  args.push_back(arg);
  PrototypeAST *proto = new PrototypeAST(name, args);
  Gen->Generate(proto);
}

static void CreateBuiltIn(string name) {
  vector<string> args;
  PrototypeAST *proto = new PrototypeAST(name, args);
  Gen->Generate(proto);
}

void *InitBuiltIns(Codegen *gen) {
  Gen = gen;
  CreateBuiltIn("sin", "a");
  CreateBuiltIn("cos", "a");
  CreateBuiltIn("exit");
}

#endif

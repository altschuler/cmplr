#include "Codegen.hpp"

using namespace llvm;
using namespace std;

Codegen::Codegen(ExecutionEngine *execEngine, Module *module) : Builder(getGlobalContext()) {	
  InitializeNativeTarget();

  TheModule = module;
  ExecEngine = execEngine;

  // Set up function optimization
  TheFPM = new FunctionPassManager(TheModule);
  TheFPM->add(new DataLayout(*ExecEngine->getDataLayout()));
  TheFPM->add(createBasicAliasAnalysisPass());
  TheFPM->add(createInstructionCombiningPass());
  TheFPM->add(createReassociatePass());
  TheFPM->add(createGVNPass());
  TheFPM->add(createCFGSimplificationPass());
  TheFPM->doInitialization();
}

Value *Codegen::Generate(ExprAST *expr) {
  switch (expr->GetASTType()) {
  case ASTNumberExpr: return this->Generate((NumberExprAST *)expr); break;
  case ASTVariableExpr: return this->Generate((VariableExprAST *)expr); break;
  case ASTBinaryExpr: return this->Generate((BinaryExprAST *)expr); break;
  case ASTCallExpr: return this->Generate((CallExprAST *)expr); break;
  case ASTConditionalExpr: return this->Generate((ConditionalExprAST *)expr); break;
  case ASTPrototype: return this->Generate((PrototypeAST *)expr); break;
  case ASTFunction: return this->Generate((FunctionAST *)expr); break;
  case ASTForExpr: return this->Generate((ForExprAST *)expr); break;
  case ASTOperator: return this->Generate((OperatorAST *)expr); break;
  }
}

Value *Codegen::Generate(NumberExprAST *expr) {
  return ConstantFP::get(getGlobalContext(), APFloat(expr->GetVal()));
};

Value *Codegen::Generate(VariableExprAST *expr) {
  using namespace boost;
  string name = expr->GetName();
  Value *val = NamedValues[name];

  if (val)
	return val;
  else {
	return ErrorV(str(format("Unknown variable '%1%'") % name).c_str());
  }
};

Value *Codegen::Generate(BinaryExprAST *expr) {
  using namespace boost;

  Value *L = this->Generate(expr->GetLHS());
  Value *R = this->Generate(expr->GetRHS());

  if (L == 0 || R == 0)
	return 0;

  switch (expr->GetOp()) {
  case '+': return Builder.CreateFAdd(L, R, "addtmp");
  case '-': return Builder.CreateFSub(L, R, "subtmp");
  case '*': return Builder.CreateFMul(L, R, "multmp");
  case '/': return Builder.CreateFDiv(L, R, "divtmp");
  case '<': 
	L = Builder.CreateFCmpULT(L, R, "cmptmp");
	return Builder.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()), "booltmp");
  default: break;
  }

  Function *opFunc = TheModule->getFunction("userop" + expr->GetOp());
  if (!opFunc) 
	return ErrorV(str(format("Unknown operator '%1%'") % expr->GetOp()).c_str());

  Value *args[2] = {L, R};
  return Builder.CreateCall(opFunc, args, "binop");
};

Value *Codegen::Generate(CallExprAST *expr) {
  using namespace boost;

  string callee = expr->GetCallee();
  vector<ExprAST*> args = expr->GetArgs();

  Function *CalleeF = TheModule->getFunction(callee);
  if (CalleeF == 0) 
	return ErrorV(formatErr("Unknown function '%1%'", callee.c_str()));

  if (CalleeF->arg_size() != args.size())
	return ErrorV(str(format("Wrong number of arguments in function %1%; got %2%, %3% expected") 
					  % args.size() 
					  % CalleeF->arg_size()  
					  % callee).c_str());

  vector<Value*> ArgsV;
  for (unsigned i = 0, e = args.size(); i != e; ++i) {
	Value *arg = this->Generate(args[i]);
	ArgsV.push_back(arg);
	if (ArgsV.back() == 0)
	  return 0;
  }

  ArrayRef<Value*> *argArr = new ArrayRef<Value*>(ArgsV);
  return Builder.CreateCall(CalleeF, *argArr, "tmpcall");
};

Value *Codegen::Generate(ConditionalExprAST *expr) {
  Value *condVal = this->Generate(expr->GetCond());
  if (condVal == 0)
	return 0;

  condVal = Builder.CreateFCmpONE(condVal, ConstantFP::get(getGlobalContext(), APFloat(0.0)), "ifcond");

  Function *func = Builder.GetInsertBlock()->getParent();

  BasicBlock *thenBlock = BasicBlock::Create(getGlobalContext(), "then", func);
  BasicBlock *elseBlock = BasicBlock::Create(getGlobalContext(), "else");
  BasicBlock *mergeBlock = BasicBlock::Create(getGlobalContext(), "merge");

  Builder.CreateCondBr(condVal, thenBlock, elseBlock);

  Builder.SetInsertPoint(thenBlock);
  
  Value *thenVal = this->Generate(expr->GetThen());
  if (thenVal == 0)
	return 0;

  Builder.CreateBr(mergeBlock);

  thenBlock = Builder.GetInsertBlock();
  
  func->getBasicBlockList().push_back(elseBlock);
  Builder.SetInsertPoint(elseBlock);
  
  Value *elseVal = this->Generate(expr->GetElse());
  if (elseVal == 0)
	return 0;

  Builder.CreateBr(mergeBlock);
  elseBlock = Builder.GetInsertBlock();

  func->getBasicBlockList().push_back(mergeBlock);
  Builder.SetInsertPoint(mergeBlock);

  PHINode *phi = Builder.CreatePHI(Type::getDoubleTy(getGlobalContext()), 2, "iftmp");
  phi->addIncoming(thenVal, thenBlock);
  phi->addIncoming(elseVal, elseBlock);

  return phi;
};

Value *Codegen::Generate(ForExprAST *expr) {
  Value *initVal = this->Generate(expr->GetInit());
  if (initVal == 0)
	return 0;

  Function *func = Builder.GetInsertBlock()->getParent();
  BasicBlock *preHeaderBlock = Builder.GetInsertBlock();
  BasicBlock *loopBlock = BasicBlock::Create(getGlobalContext(), "loop", func);

  Builder.CreateBr(loopBlock);
  
  // set insert to the loop
  Builder.SetInsertPoint(loopBlock);

  // create the phi node and set initVal as entry
  PHINode *phi = Builder.CreatePHI(Type::getDoubleTy(getGlobalContext()), 2, expr->GetIterName());
  phi->addIncoming(initVal, preHeaderBlock);

  // save old value of same name as IterName (if any)
  Value *oldVal = NamedValues[expr->GetIterName()];
  NamedValues[expr->GetIterName()] = phi;

  // emit body code
  if (this->Generate(expr->GetBody()) == 0)
	return 0;

  // handle step
  Value *stepVal;
  if (expr->GetStep()) {
	stepVal = this->Generate(expr->GetStep());
	if (stepVal == 0)
	  return 0;
  } 
  // if step is not specified, use ++
  else {
	stepVal = ConstantFP::get(getGlobalContext(), APFloat(1.0));
  }

  Value *nextVal = Builder.CreateFAdd(phi, stepVal, "nextvar");

  Value *endCond = this->Generate(expr->GetEnd());
  if (endCond == 0)
	return 0;
  // booleanize end condition
  endCond = Builder.CreateFCmpONE(endCond, ConstantFP::get(getGlobalContext(), APFloat(0.0)), "loopcond");

  BasicBlock *loopEndBlock = Builder.GetInsertBlock();
  BasicBlock *afterBlock = BasicBlock::Create(getGlobalContext(), "afterloop", func);
  
  // create the loop ending branch
  Builder.CreateCondBr(endCond, loopEndBlock, afterBlock);
  
  // start inserting code after loop
  Builder.SetInsertPoint(afterBlock);

  phi->addIncoming(nextVal, loopEndBlock);

  // restore the saved variable
  if (oldVal) 
	NamedValues[expr->GetIterName()] = oldVal;
  else
	NamedValues.erase(expr->GetIterName());

  return Constant::getNullValue(Type::getDoubleTy(getGlobalContext()));
}

Function *Codegen::Generate(PrototypeAST *proto) {
  string Name = proto->GetName();
  vector<string> Args = proto->GetArgs();

  vector<Type*> Doubles(Args.size(), Type::getDoubleTy(getGlobalContext()));
  FunctionType *FT = FunctionType::get(Type::getDoubleTy(getGlobalContext()), Doubles, false);
  Function* F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule);
  

  if (F->getName() != Name) 
	{
	  F->eraseFromParent();
	  F = TheModule->getFunction(Name);
	
	  if (!F->empty()) 
		{
		  ErrorF("Redefinition of function");
		  return 0;
		}
	
	  if (F->arg_size() != Args.size()) 
		{
		  Error("Redefinition of function with wrong number of arguments");
		  return 0;
		}
	}
  
  unsigned Idx = 0;
  for (Function::arg_iterator AI = F->arg_begin(); Idx != Args.size(); ++AI, ++Idx)
	{
	  AI->setName(Args[Idx]);
	  NamedValues[Args[Idx]] = AI;
	}

  return F;
};


Function *Codegen::Generate(FunctionAST *funcAst) {
  NamedValues.clear();

  Function *func = this->Generate(funcAst->GetPrototype());
  if (func == 0)
	return 0;
	
  BasicBlock *block = BasicBlock::Create(getGlobalContext(), "func", func);
  Builder.SetInsertPoint(block);
	
  Value *retVal = this->Generate(funcAst->GetBody());
  if (retVal) {
	Builder.CreateRet(retVal);
	verifyFunction(*func);
	TheFPM->run(*func);
	return func;
  }
	
  func->eraseFromParent();
  return 0;
};

Function *Codegen::Generate(OperatorAST *opr) {
  NamedValues.clear();

  string Name = "userop" + opr->GetOp();
  vector<string> Args = opr->GetArgs();

  vector<Type*> Doubles(Args.size(), Type::getDoubleTy(getGlobalContext()));
  FunctionType *FT = FunctionType::get(Type::getDoubleTy(getGlobalContext()), Doubles, false);
  Function* F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule);
  
  if (F->getName() != Name) {
	F->eraseFromParent();
	F = TheModule->getFunction(Name);
	
	if (!F->empty()) {
	  ErrorF("Redefinition of operator");
	  return 0;
	}
	
	if (F->arg_size() != Args.size()) {
	  Error("Redefinition of operator with wrong number of arguments");
	  return 0;
	}
  }
  
  unsigned Idx = 0;
  for (Function::arg_iterator AI = F->arg_begin(); Idx != Args.size(); ++AI, ++Idx) {
	AI->setName(Args[Idx]);
	NamedValues[Args[Idx]] = AI;
  }
  
  // add the body 
  BasicBlock *block = BasicBlock::Create(getGlobalContext(), "opfunc", F);
  Builder.SetInsertPoint(block);

  Value *retVal = this->Generate(opr->GetBody());
  if (retVal) {
	Builder.CreateRet(retVal);
	verifyFunction(*F);
	TheFPM->run(*F);
	return F;
  }
	
  F->eraseFromParent();
  return 0;
};

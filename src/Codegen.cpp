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
  TheFPM->add(createPromoteMemoryToRegisterPass());
  TheFPM->add(createInstructionCombiningPass());
  TheFPM->add(createReassociatePass());
  TheFPM->add(createGVNPass());
  TheFPM->add(createCFGSimplificationPass());
  TheFPM->doInitialization();
}

AllocaInst *Codegen::CreateEntryBlockAlloca(Function *func, string varName) {
	IRBuilder<> builder(&func->getEntryBlock(), func->getEntryBlock().begin());
	return builder.CreateAlloca(Type::getDoubleTy(getGlobalContext()), 0, varName.c_str());
};

Value *Codegen::Generate(ExprAST *expr) {
  switch (expr->GetASTType()) {
  case ASTNumberExpr: return this->Generate((NumberExprAST *)expr); break;
  case ASTVariableExpr: return this->Generate((VariableExprAST *)expr); break;
  case ASTBinaryExpr: return this->Generate((BinaryExprAST *)expr); break;
  case ASTCallExpr: return this->Generate((CallExprAST *)expr); break;
  case ASTConditionalExpr: return this->Generate((ConditionalExprAST *)expr); break;
  case ASTForExpr: return this->Generate((ForExprAST *)expr); break;
  case ASTUnary: return this->Generate((UnaryExprAST *)expr); break;
  case ASTPrototype: return this->Generate((PrototypeAST *)expr); break;
  case ASTFunction: return this->Generate((FunctionAST *)expr); break;
  case ASTOperator: return this->Generate((OperatorAST *)expr); break;
  case ASTBlock: return 0;
  }
}

Value *Codegen::Generate(NumberExprAST *expr) {
  return ConstantFP::get(getGlobalContext(), APFloat(expr->GetVal()));
};

Value *Codegen::Generate(VariableExprAST *expr) {
  string name = expr->GetName();
  AllocaInst *alloca = NamedValues[name];

  if (alloca == 0)
	return BaseError::Throw<Value*>(str(boost::format("Unknown variable '%1%'") % name).c_str());

  return Builder.CreateLoad(alloca, name.c_str()); // XXX remove c_str
};

Value *Codegen::Generate(BinaryExprAST *expr) {
  using namespace boost;

  // treat assignment separately
  if (expr->GetOp() == '=') {
	  VariableExprAST *identifier = dynamic_cast<VariableExprAST*>(expr->GetLHS());
	  if (!identifier)
		  return BaseError::Throw<Value*>("Left hand of assignment must be a variable");

	  Value *val = this->Generate(expr->GetRHS());
	  if (!val)
		  return BaseError::Throw<Value*>("Invalid assignment value to variable");

	  Value *variable = NamedValues[identifier->GetName()];
	  if (!variable)
		  return BaseError::Throw<Value*>("Unknown variable, cannot assign");

	  Builder.CreateStore(val, variable);
	  return val;
  }

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

  Function *opFunc = TheModule->getFunction("binary" + expr->GetOp());
  if (!opFunc) 
	return BaseError::Throw<Value*>(str(format("Unknown binary operator '%1%'") % expr->GetOp()));

  Value *args[2] = {L, R};
  return Builder.CreateCall(opFunc, args, "binop");
};

Value *Codegen::Generate(CallExprAST *expr) {
  using namespace boost;

  string callee = expr->GetCallee();
  vector<ExprAST*> args = expr->GetArgs();

  Function *CalleeF = TheModule->getFunction(callee);
  if (CalleeF == 0) 
	return BaseError::Throw<Value*>(str(format("Unknown function '%1%'") % callee));

  if (CalleeF->arg_size() != args.size())
	return BaseError::Throw<Value*>(str(boost::format("Wrong number of arguments in function %1%; got %2%, %3% expected")
						% callee.c_str()
						% args.size()
						% CalleeF->arg_size()));

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
	vector<ConditionalElement*> conds = expr->GetConds();

	Function *func = Builder.GetInsertBlock()->getParent();
	BasicBlock *mergeBlock = BasicBlock::Create(getGlobalContext(), "merge");
	BasicBlock *entryBlock = Builder.GetInsertBlock();

	// return value will be determined using a phi node in the merge block
	Builder.SetInsertPoint(mergeBlock);
	PHINode *phi = Builder.CreatePHI(Type::getDoubleTy(getGlobalContext()), conds.size(), "iftmp");

	Builder.SetInsertPoint(entryBlock);

	for (int i = 0; i < conds.size(); ++i) {
		ConditionalElement *elm = conds[i];

		// emit condition
		Value *condVal = this->Generate(elm->GetCond());
		condVal = Builder.CreateFCmpONE(condVal, ConstantFP::get(getGlobalContext(), APFloat(0.0)), "ifcond");
		if (condVal == 0)
			return BaseError::Throw<Value*>("Condition is undefined");

		BasicBlock *condBlock = BasicBlock::Create(getGlobalContext(), "then");
		BasicBlock *nextBlock = BasicBlock::Create(getGlobalContext(), "else");

		// branch into body if condition is met, else skip to next conditional element
		Builder.CreateCondBr(condVal, condBlock, nextBlock);

		// emit body code into conditional block
		Builder.SetInsertPoint(condBlock);
		Value *condBody = this->Generate(elm->GetConsequence());
		// merge back into main flow
		Builder.CreateBr(mergeBlock);

		// add value to phi
		phi->addIncoming(condBody, condBlock);

		// push conditional block into func
		func->getBasicBlockList().push_back(condBlock);
		func->getBasicBlockList().push_back(nextBlock);

		Builder.SetInsertPoint(nextBlock);
	}

	// 'else' block
	Value *elseVal = this->Generate(expr->GetElse());
	if (elseVal == 0)
		return BaseError::Throw<Value*>("'else' value is undefined");

	Builder.CreateBr(mergeBlock);
	//elseBlock = Builder.GetInsertBlock();

	// add else value to phi
	phi->addIncoming(elseVal, Builder.GetInsertBlock());

	func->getBasicBlockList().push_back(mergeBlock);

	Builder.SetInsertPoint(mergeBlock);

//	func->viewCFG();

	return phi;
};

Value *Codegen::Generate(ForExprAST *expr) {

	string iterName = expr->GetIterName();
  Function *func = Builder.GetInsertBlock()->getParent();

  // create an alloca for the iterated variable
  AllocaInst *alloca = this->CreateEntryBlockAlloca(func, iterName.c_str());

  Value *initVal = this->Generate(expr->GetInit());
  if (initVal == 0)
	return 0;

  Builder.CreateLoad(initVal, iterName.c_str());

  BasicBlock *preHeaderBlock = Builder.GetInsertBlock();
  BasicBlock *loopBlock = BasicBlock::Create(getGlobalContext(), "loop", func);

  Builder.CreateBr(loopBlock);
  Builder.SetInsertPoint(loopBlock);

  // save old value of same name as IterName (if any)
  AllocaInst *oldVal = NamedValues[iterName];
  NamedValues[iterName] = alloca;

  // emit body code into loop block
  Value* bodyVal =  this->Generate(expr->GetBody());
  if (bodyVal == 0)
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


  Value *endCond = this->Generate(expr->GetEnd());
  if (endCond == 0)
	return 0;

  Value *currentVal = Builder.CreateLoad(alloca, iterName.c_str());
  Value *nextVal = Builder.CreateFAdd(currentVal, stepVal, "nextvar");
  Builder.CreateStore(nextVal, alloca);

  // booleanize end condition
  endCond = Builder.CreateFCmpONE(endCond, ConstantFP::get(getGlobalContext(), APFloat(0.0)), "loopcond");

  BasicBlock *loopEndBlock = Builder.GetInsertBlock();
  BasicBlock *afterBlock = BasicBlock::Create(getGlobalContext(), "afterloop", func);
  
  // create the loop ending branch
  Builder.CreateCondBr(endCond, loopBlock, afterBlock);
  
  // start inserting code after loop
  Builder.SetInsertPoint(afterBlock);

  //phi->addIncoming(nextVal, loopEndBlock);

  // restore the saved variable
  if (oldVal) 
	NamedValues[iterName] = oldVal;
  else
	NamedValues.erase(iterName);

  return bodyVal;
  //return Constant::getNullValue(Type::getDoubleTy(getGlobalContext()));
}

Function *Codegen::Generate(PrototypeAST *proto) {
  string funcName = proto->GetName();
  vector<string> args = proto->GetArgs();

  vector<Type*> Doubles(args.size(), Type::getDoubleTy(getGlobalContext()));
  FunctionType *funcType = FunctionType::get(Type::getDoubleTy(getGlobalContext()), Doubles, false);
  Function* func = Function::Create(funcType, Function::ExternalLinkage, funcName, TheModule);
  

  if (func->getName() != funcName) 
	{
	  func->eraseFromParent();
	  func = TheModule->getFunction(funcName);
	
	  if (!func->empty()) 
		{
		  BaseError::Throw<Function*>("Redefinition of function");
		  return 0;
		}
	
	  if (func->arg_size() != args.size()) 
		{
		  BaseError::Throw<Function*>("Redefinition of function with wrong number of arguments");
		  return 0;
		}
	}
  
  unsigned Idx = 0;
  for (Function::arg_iterator AI = func->arg_begin(); Idx != args.size(); ++AI, ++Idx)
	{
	  AllocaInst *alloca = this->CreateEntryBlockAlloca(func, args[Idx]);
	  Builder.CreateStore(AI, alloca);
	  NamedValues[args[Idx]] = alloca;
	}

  return func;
};


Function *Codegen::Generate(FunctionAST *funcAst) {
  NamedValues.clear();

  Function *func = this->Generate(funcAst->GetPrototype());
  if (func == 0)
	return 0;
	
  BasicBlock *block = BasicBlock::Create(getGlobalContext(), "entry", func);
  Builder.SetInsertPoint(block);

  // iterate and codegen all expressions in body  
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

Value *Codegen::Generate(BlockAST *block) {
  vector<ExprAST*> exprs = block->GetExpressions();
  int size = exprs.size();
  for (int i = 0; i < size; ++i) {
	ExprAST *curr = exprs[i];
	Value *val = this->Generate(curr);

	// create return value if last expression
	if (i == size - 1)
	  return val;
  }  
  
  return 0;
}

Value *Codegen::Generate(UnaryExprAST *expr) {
  ExprAST *code = expr->GetOperand();
  Value *val = this->Generate(code);
  if(!val)
	return 0;

  Function *func = TheModule->getFunction("unary" + expr->GetOp());
  if (func == 0)
	return BaseError::Throw<Value*>("Unknown unary operator");

  return Builder.CreateCall(func, val, "unaryop");
}

Function *Codegen::Generate(OperatorAST *opr) {
  NamedValues.clear();

  string opName = (opr->IsBinary() ? "binary" : "unary") + opr->GetOp();
  vector<string> args = opr->GetArgs();

  vector<Type*> Doubles(args.size(), Type::getDoubleTy(getGlobalContext()));
  FunctionType *funcType = FunctionType::get(Type::getDoubleTy(getGlobalContext()), Doubles, false);
  Function* func = Function::Create(funcType, Function::ExternalLinkage, opName, TheModule);
  
  if (func->getName() != opName) {
	func->eraseFromParent();
	func = TheModule->getFunction(opName);
	
	if (!func->empty()) {
		BaseError::Throw<Function*>("Redefinition of operator");
	  return 0;
	}
	
	if (func->arg_size() != args.size()) {
		BaseError::Throw<Function*>("Redefinition of operator with wrong number of arguments");
	  	return 0;
	}
  }
  
  unsigned Idx = 0;
  for (Function::arg_iterator AI = func->arg_begin(); Idx != args.size(); ++AI, ++Idx) {
	  AllocaInst *alloca = this->CreateEntryBlockAlloca(func, opName);
	  Builder.CreateStore(AI, alloca);
	NamedValues[args[Idx]] = alloca;
  }
  
  // add the body 
  BasicBlock *block = BasicBlock::Create(getGlobalContext(), "opfunc", func);
  Builder.SetInsertPoint(block);

  Value *retVal = this->Generate(opr->GetBody());
  if (retVal) {
	Builder.CreateRet(retVal);
	verifyFunction(*func);
	TheFPM->run(*func);
	return func;
  }
	
  func->eraseFromParent();
  return 0;
};

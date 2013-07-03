#include "Driver.hpp"

void Driver::HandleDefinition() {
	FunctionAST *func = TheParser.ParseDefinition();
	Function *code = Gen->Generate(func);
	if (!(func && code))
		TheParser.GetNextToken();
}

void Driver::HandleImport() {
	ImportAST *imp = TheParser.ParseImport();

	Driver *driver = new Driver(Gen);
	driver->Go(imp->FileName + ".wtf");

	// set lexer back to this drivers parser's, to give correct debug locations
	BaseError::SetLexer(TheParser.GetLexer());

	TheParser.GetNextToken();
}

void Driver::HandleOperator() {
	OperatorAST *func = TheParser.ParseOperator();
	Function *code = Gen->Generate(func);
	if (!(func && code))
		TheParser.GetNextToken();
}

void Driver::HandleExtern() {
	PrototypeAST *ext = TheParser.ParseExtern();
	Function *code = Gen->Generate(ext);
	// try recovering by ignoring current token
	if (!(ext && code))
		TheParser.GetNextToken();
}

void Driver::HandleTopLevelExpr() {
	FunctionAST *expr = TheParser.ParseTopLevelExpr();
	Function *code = Gen->Generate(expr);
	if (expr && code) {
		llvm::ExecutionEngine* execEngine = Gen->GetExecEngine();
		// execute the anonymous wrapper function
		void *funcPtr = execEngine->getPointerToFunction(code);
		double (*fptr)() = (double (*)())(intptr_t)funcPtr;
		fptr();
	} else
		TheParser.GetNextToken();
}

void Driver::Go(string file) {
	CurrentFile = file;
	TheParser.SetInputFile(file, 0);

	TheParser.GetNextToken();

	while (1) {
		switch (TheParser.GetCurTok()) {
		case tok_eof:
			return;
		case tok_def:
			HandleDefinition();
			break;
		case tok_extern:
			HandleExtern();
			break;
		case tok_op:
			HandleOperator();
			break;
		case tok_import:
			HandleImport();
			break;
		case tok_end:
		case ';':
			TheParser.GetNextToken(); // eat ';' or 'end'
			break;
		default:
			HandleTopLevelExpr();
			break;
		}
	}
}

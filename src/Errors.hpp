#include "llvm/Value.h"
#include "boost/format.hpp"
#include "Lexer.hpp"
#include "AST.hpp"

#ifndef ERRORS_HPP
#define ERRORS_HPP

class BaseError {
	static Lexer *Lex;
public:
	static void SetLexer(Lexer *lexer) { BaseError::Lex = lexer; };

	template<class T>
	static T Throw(string message) {
		fprintf(stderr, "Error: %s, in %s:%i:%i\n",
				message.c_str(),
				BaseError::Lex->GetFile().c_str(),
				BaseError::Lex->GetCursorLinePosition(),
				BaseError::Lex->GetCursorColumnPosition());
		return 0;
	}
};

// Error helpers
ExprAST *Error(const char *Str);
PrototypeAST *ErrorP(const char *Str);
FunctionAST *ErrorF(const char *Str);
OperatorAST *ErrorO(const char *Str);
Value *ErrorV(const char *Str);
ImportAST *ErrorI(const char *Str);
BlockAST *ErrorB(const char *Str);

#endif


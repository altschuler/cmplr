#include "Lexer.hpp"

#ifndef LEXER_H
#define LEXER_H

using namespace std;

void Lexer::SetInputFile(string file, int initialSeek) {
	File = file;
	InputFileStream.open(file);

	if ( !InputFileStream)
	BaseError::Throw<int>(str(boost::format("File not found: '%1%'") % file));

	CursorPosition = initialSeek;
	InputFileStream.seekg(CursorPosition);
}

int Lexer::GetNextChar() {
	CursorPosition++;
	CursorColumnPosition++;

	char ch = InputFileStream.get();
	if (ch == '\n') {
		CursorLinePosition++;
		CursorColumnPosition = 0;
	}
	return ch;
}

int Lexer::GetToken() {
	while (isspace(LastChar))
		LastChar = this->GetNextChar();

	// check identifier
	if (isalpha(LastChar)) {
		IdentifierStr = LastChar;
		while (isalnum(LastChar = this->GetNextChar()))
			IdentifierStr += LastChar;

		if (IdentifierStr == "func") return tok_func;
		if (IdentifierStr == "extern") return tok_extern;
		if (IdentifierStr == "if") return tok_if;
		if (IdentifierStr == "then") return tok_then;
		if (IdentifierStr == "else") return tok_else;
		if (IdentifierStr == "elsif") return tok_elsif;
		if (IdentifierStr == "for") return tok_for;
		if (IdentifierStr == "in") return tok_in;
		if (IdentifierStr == "op") return tok_op;
		if (IdentifierStr == "import") return tok_import;
		if (IdentifierStr == "end") return tok_end;
		if (IdentifierStr == "var") return tok_var;

		return tok_identifier;
	}

	// check number literal
	if (isdigit(LastChar) || LastChar == '.') {
		string NumStr;
		do {
			NumStr += LastChar;
			LastChar = this->GetNextChar();
		}
		while (isdigit(LastChar) || LastChar == '.');

		this->NumVal = strtod(NumStr.c_str(), 0);
		return tok_number;
	}

	// check for comment
	if (LastChar == '#') {
		do
			LastChar = this->GetNextChar();
		while (LastChar != '\n' && LastChar != EOF);

		if (LastChar != EOF)
		return GetToken();
	}

	// parse string
	if (LastChar == '\'') {
		// eat '{'
		LastChar = this->GetNextChar();

		while (LastChar != '\'') {
			StringVal += LastChar;
			LastChar = this->GetNextChar();
		}

		// eat '}'
		LastChar = this->GetNextChar();

		return tok_string;
	}

	if (LastChar == EOF)
	return tok_eof;

	int ThisChar = LastChar;
	LastChar = this->GetNextChar();
	return ThisChar;
}
#endif

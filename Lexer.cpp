#include <cstdio>
#include <cstdlib>
#include <string>
#include "Lexer.hpp"

#ifndef LEXER_H
#define LEXER_H

using namespace std;

void Lexer::SetInputFile(string file) {
  InputFileStream.open(file);
}

int Lexer::GetNextChar() {
  return InputFileStream.get();
}

int Lexer::GetToken() {
  static int LastChar = ' ';
  
  while (isspace(LastChar))
	LastChar = this->GetNextChar();

  // check identifier
  if (isalpha(LastChar)) {
	IdentifierStr = LastChar;
	while (isalnum(LastChar = this->GetNextChar())) 
	  IdentifierStr += LastChar;

	if (IdentifierStr == "def") return tok_def;
	if (IdentifierStr == "extern") return tok_extern;
	if (IdentifierStr == "if") return tok_if;
	if (IdentifierStr == "then") return tok_then;
	if (IdentifierStr == "else") return tok_else;
	if (IdentifierStr == "for") return tok_for;
	if (IdentifierStr == "in") return tok_in;
	if (IdentifierStr == "op") return tok_op;

	return tok_identifier;
  }

  // check number literal
  if (isdigit(LastChar) || LastChar == '.') {
	string NumStr;
	do {
	  NumStr += LastChar;
	  LastChar = this->GetNextChar();
	} while (isdigit(LastChar) || LastChar == '.');

   	this->NumVal = strtod(NumStr.c_str(), 0);
	return tok_number;
  }

  // check for comment
  if (LastChar == '#') {
	do LastChar = this->GetNextChar();
	while (LastChar != '\n' && LastChar != EOF);

	if (LastChar != EOF)
	  return GetToken();
  }

  if (LastChar == EOF)
	return tok_eof;

  int ThisChar = LastChar;
  LastChar = this->GetNextChar();
  return ThisChar;
}
#endif

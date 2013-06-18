#include <cstdio>
#include <cstdlib>
#include <string>
#include "Lexer.hpp"

#ifndef LEXER_H
#define LEXER_H

using namespace std;

Lexer::Lexer () {
  IdentifierStr = "";
  NumVal = 0.0;
}

int Lexer::GetToken() {
  static int LastChar = ' ';
  
  while (isspace(LastChar))
	LastChar = getchar();

  // check identifier
  if (isalpha(LastChar)) {
	IdentifierStr = LastChar;
	while (isalnum(LastChar = getchar())) 
	  IdentifierStr += LastChar;

	if (IdentifierStr == "def") return tok_def;
	if (IdentifierStr == "extern") return tok_extern;
	return tok_identifier;
  }

  // check number literal
  if (isdigit(LastChar) || LastChar == '.') {
	string NumStr;
	do {
	  NumStr += LastChar;
	  LastChar = getchar();
	} while (isdigit(LastChar) || LastChar == '.');

   	this->NumVal = strtod(NumStr.c_str(), 0);
	return tok_number;
  }

  // check for comment
  if (LastChar == '#') {
	do LastChar = getchar();
	while (LastChar != '\n' && LastChar != EOF);

	if (LastChar != EOF)
	  return GetToken();
  }

  if (LastChar == EOF)
	return tok_eof;

  int ThisChar = LastChar;
  LastChar = getchar();
  return ThisChar;
}
#endif

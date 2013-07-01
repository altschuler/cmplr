#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>

using namespace std;

#ifndef LEXER_HPP
#define LEXER_HPP

enum Token {
	tok_eof = -1,
	tok_def = -2,
	tok_extern = -3,
	tok_identifier = -4,
	tok_number = -5,
	tok_if = -6,
	tok_then = -7,
	tok_else = -8,
	tok_for = -9,
	tok_in = -10,
	tok_op = -11,
	tok_import = -12,
	tok_string = -13,
	tok_end = -14,
};

class Lexer {
	int LastChar;
	string File;
	ifstream InputFileStream;
	string IdentifierStr;
	string StringVal;
	double NumVal;

	int CursorPosition;
	int CursorColumnPosition;
	int CursorLinePosition;

public:
	Lexer() : LastChar(' '), IdentifierStr("") {}

	void SetInputFile(string file, int initialSeek);

	int GetToken();

	string GetIdentifierStr() {
		return IdentifierStr;
	}
	double GetNumVal() {
		return NumVal;
	}
	string GetFile() {
		return File;
	}
	string GetStringVal() {
		return StringVal;
	}
	int GetCursorPosition() {
		return CursorPosition;
	}
	int GetCursorColumnPosition() {
		return CursorColumnPosition;
	}
	int GetCursorLinePosition() {
		return CursorLinePosition;
	}

private:
	int GetNextChar();
};

#endif
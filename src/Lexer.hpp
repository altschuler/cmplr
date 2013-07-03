#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>

using namespace std;

#ifndef LEXER_HPP
#define LEXER_HPP

enum Token {
  tok_eof = -2,
  tok_import = -4,

  tok_def = -8,
  tok_op = -16,
  tok_end = -32,

  tok_extern = -64,

  tok_identifier = -128,
  tok_number = -256,
  tok_string = -512,

  tok_if = -1024,
  tok_then = -2048,
  tok_else = -4096,
  tok_elsif = -8192,

  tok_for = -16384,
  tok_in = -32768,
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
  Lexer() : LastChar(' '), IdentifierStr(""), CursorLinePosition(0) {}

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

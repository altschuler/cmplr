#include <cstdio>
#include <cstdlib>
#include <string>

using namespace std;

enum Token {
  tok_eof = -1,
  tok_def = -2, tok_extern = -3,
  tok_identifier = -4, tok_number = -5,
  tok_if = -6, tok_then = -7, tok_else = -8,
};

class Lexer {
  string IdentifierStr;
  double NumVal;

public:
  Lexer();
  int GetToken();

  string GetIdentifierStr() { return IdentifierStr; }
  double GetNumVal()  { return NumVal; }
};

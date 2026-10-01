#ifndef COMMON_H
#define COMMON_H

#include <string>
using namespace std;

enum TokenKind { TK_EOF, TK_ID, TK_KW, TK_CNST, TK_OP, TK_SEP, TK_ERR };

struct Token {
    TokenKind kind;
    int index;
    int line, col;
    string lexeme;
};

bool isNamedConstLexeme(const string& s);

#endif
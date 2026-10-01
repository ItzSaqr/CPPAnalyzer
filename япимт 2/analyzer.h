#ifndef ANALYZER_H
#define ANALYZER_H

#include <vector>
#include <string>
#include <fstream>
#include "common.h"

using namespace std;

extern vector<string> postfix;

class MutableTable;
class StaticTable;

class Analyzer {
private:
    vector<Token> tokens;
    int pos;
    MutableTable& table;
    ofstream& err;

    bool isAtEnd() const;
    Token current() const;
    Token advance();
    bool check(TokenKind kind, const string& lexeme = "") const;
    bool match(TokenKind kind, const string& lexeme = "");
    void SEMerrorHere(const string& msg);
    void SYNerrorHere(const string& msg);
    void SEMerrorToken(const Token& t, const string& msg);
    void SYNerrorToken(const Token& t, const string& msg);
    void synchronize();
    bool expect(TokenKind kind, const string& msg, const string& lexeme = "");

    int parsePrimary();
    int parseUnary();
    int parseShift();
    int parseMul();
    int parseAdd();
    int parseRel();
    int parseEq();
    int parseBitwiseAnd();
    int parseBitwiseXor();
    int parseBitwiseOr();
    int parseAnd();
    int parseOr();
    int parseExpression();

    void parseConstDeclaration();
    void parseDeclaration();
    void parseAssignment();
    void parseStatement();

public:
    Analyzer(const vector<Token>& toks, MutableTable& tbl, ofstream& e);
    void analyzeProgram();
};

vector<Token> readTokensFromData(const string& datFile,
    StaticTable& kw, StaticTable& op, StaticTable& sep,
    MutableTable& table);

void analyze(const string& datFile, const string& semErrFile,
    StaticTable& kw, StaticTable& op, StaticTable& sep,
    MutableTable& table);

#endif
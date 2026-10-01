#include "analyzer.h"
#include "tables.h"


bool Analyzer::isAtEnd() const {
    return pos >= (int)tokens.size();
}

Token Analyzer::current() const {
    if (isAtEnd()) return { TK_EOF, -1, -1, -1, "" };
    return tokens[pos];
}

Token Analyzer::advance() {
    if (!isAtEnd()) pos++;
    return tokens[pos - 1];
}

bool Analyzer::check(TokenKind kind, const string& lexeme) const {
    if (isAtEnd()) return false;
    if (tokens[pos].kind != kind) return false;
    if (!lexeme.empty() && tokens[pos].lexeme != lexeme) return false;
    return true;
}

bool Analyzer::match(TokenKind kind, const string& lexeme) {
    if (!check(kind, lexeme)) return false;
    advance();
    return true;
}

void Analyzer::SEMerrorHere(const string& msg) {
    Token t = current();
    err << "SEM_ERR line=" << t.line
        << " col=" << t.col
        << " : " << msg << "\n";
}

void Analyzer::SYNerrorHere(const string& msg) {
    Token t = current();
    err << "SYN_ERR line=" << t.line
        << " col=" << t.col
        << " : " << msg << "\n";
}

void Analyzer::SEMerrorToken(const Token& t, const string& msg) {
    err << "SEM_ERR line=" << t.line
        << " col=" << t.col
        << " : " << msg << "\n";
}

void Analyzer::SYNerrorToken(const Token& t, const string& msg) {
    err << "SYN_ERR line=" << t.line
        << " col=" << t.col
        << " : " << msg << "\n";
}

void Analyzer::synchronize() {
    while (!isAtEnd()) {
        if (check(TK_SEP, ";")) {
            advance();
            return;
        }

        if (check(TK_KW, "int") || check(TK_KW, "const")) {
            return;
        }

        advance();
    }
}

bool Analyzer::expect(TokenKind kind, const string& msg, const string& lexeme) {
    if (check(kind, lexeme)) {
        advance();
        return true;
    }
    SYNerrorHere(msg);
    return false;
}

int Analyzer::parsePrimary() {
    if (check(TK_CNST)) {
        Token t = advance();
        postfix.push_back(t.lexeme);

        int val = 0;
        try {
            val = stoi(t.lexeme);
        }
        catch (...) {
            SEMerrorToken(t, "invalid integer constant");
        }
        return val;
    }

    if (check(TK_ID)) {
        Token t = advance();
        if (!table.isDeclared(t.lexeme)) {
            SEMerrorToken(t, "identifier '" + t.lexeme + "' used before declaration");
            return 0;
        }

        postfix.push_back(t.lexeme);

        int val = 0;
        if (!table.getIntValue(t.lexeme, val)) {
            return 0;
        }
        return val;
    }

    if (match(TK_SEP, "(")) {
        int val = parseExpression();
        expect(TK_SEP, "expected ')'", ")");
        return val;
    }

    SYNerrorHere("expected constant, identifier or '('");
    if (!isAtEnd()) advance();
    return 0;
}

int Analyzer::parseUnary() {
    if (match(TK_OP, "+")) {
        int v = parseUnary();
        postfix.push_back("u+");
        return v;
    }

    if (match(TK_OP, "-")) {
        int v = parseUnary();
        postfix.push_back("u-");
        return -v;
    }

    if (match(TK_OP, "!")) {
        int v = parseUnary();
        postfix.push_back("!");
        return !v;
    }

    return parsePrimary();
}

int Analyzer::parseShift() {
    int left = parseUnary();

    while (true) {
        if (match(TK_OP, "<<")) {
            int r = parseUnary();
            postfix.push_back("<<");
            left = left << r;
        }
        else if (match(TK_OP, ">>")) {
            int r = parseUnary();
            postfix.push_back(">>");
            left = left >> r;
        }
        else {
            break;
        }
    }

    return left;
}

int Analyzer::parseMul() {
    int left = parseShift();

    while (true) {
        if (match(TK_OP, "*")) {
            int r = parseShift();
            postfix.push_back("*");
            left = left * r;
        }
        else if (match(TK_OP, "/")) {
            int r = parseShift();
            postfix.push_back("/");

            if (r == 0) {
                SEMerrorToken(tokens[pos - 1], "division by zero");
                left = 0;
            }
            else {
                left = left / r;
            }
        }
        else if (match(TK_OP, "%")) {
            int r = parseShift();
            postfix.push_back("%");

            if (r == 0) {
                SEMerrorToken(tokens[pos - 1], "modulo by zero");
                left = 0;
            }
            else {
                left = left % r;
            }
        }
        else {
            break;
        }
    }

    return left;
}

int Analyzer::parseAdd() {
    int left = parseMul();

    while (true) {
        if (match(TK_OP, "+")) {
            int r = parseMul();
            postfix.push_back("+");
            left = left + r;
        }
        else if (match(TK_OP, "-")) {
            int r = parseMul();
            postfix.push_back("-");
            left = left - r;
        }
        else {
            break;
        }
    }

    return left;
}

int Analyzer::parseRel() {
    int left = parseAdd();

    while (true) {
        if (match(TK_OP, "<")) {
            int r = parseAdd();
            postfix.push_back("<");
            left = (left < r);
        }
        else if (match(TK_OP, ">")) {
            int r = parseAdd();
            postfix.push_back(">");
            left = (left > r);
        }
        else if (match(TK_OP, "<=")) {
            int r = parseAdd();
            postfix.push_back("<=");
            left = (left <= r);
        }
        else if (match(TK_OP, ">=")) {
            int r = parseAdd();
            postfix.push_back(">=");
            left = (left >= r);
        }
        else {
            break;
        }
    }

    return left;
}

int Analyzer::parseEq() {
    int left = parseRel();

    while (true) {
        if (match(TK_OP, "==")) {
            int r = parseRel();
            postfix.push_back("==");
            left = (left == r);
        }
        else if (match(TK_OP, "!=")) {
            int r = parseRel();
            postfix.push_back("!=");
            left = (left != r);
        }
        else {
            break;
        }
    }

    return left;
}

int Analyzer::parseBitwiseAnd() {
    int left = parseEq();

    while (check(TK_OP, "&")) {
        if (pos + 1 < (int)tokens.size() &&
            tokens[pos + 1].kind == TK_OP && tokens[pos + 1].lexeme == "&") {
            break;  // это &&
        }

        advance();
        int r = parseEq();
        postfix.push_back("&");
        left = left & r;
    }

    return left;
}

int Analyzer::parseBitwiseXor() {
    int left = parseBitwiseAnd();

    while (match(TK_OP, "^")) {
        int r = parseBitwiseAnd();
        postfix.push_back("^");
        left = left ^ r;
    }

    return left;
}

int Analyzer::parseBitwiseOr() {
    int left = parseBitwiseXor();

    while (check(TK_OP, "|")) {
        if (pos + 1 < (int)tokens.size() &&
            tokens[pos + 1].kind == TK_OP && tokens[pos + 1].lexeme == "|") {
            break;  // это ||
        }

        advance();
        int r = parseBitwiseXor();
        postfix.push_back("|");
        left = left | r;
    }

    return left;
}

int Analyzer::parseAnd() {
    int left = parseBitwiseOr();

    while (match(TK_OP, "&&")) {
        int r = parseBitwiseOr();
        postfix.push_back("&&");
        left = (left && r);
    }

    return left;
}

int Analyzer::parseOr() {
    int left = parseAnd();

    while (match(TK_OP, "||")) {
        int r = parseAnd();
        postfix.push_back("||");
        left = (left || r);
    }

    return left;
}

int Analyzer::parseExpression() {
    return parseOr();
}

void Analyzer::parseConstDeclaration() {
    Token constTok = advance(); // const

    if (!check(TK_KW, "int")) {
        SYNerrorHere("expected 'int' after 'const'");
        synchronize();
        return;
    }

    Token intTok = advance();

    if (intTok.line != constTok.line) {
        SEMerrorToken(intTok, "line break between 'const' and 'int' is not allowed");
    }

    if (!check(TK_ID)) {
        SYNerrorHere("expected identifier after 'const int'");
        synchronize();
        return;
    }

    Token idTok = advance();

    if (idTok.line != intTok.line) {
        SEMerrorToken(idTok, "line break after 'const int' is not allowed");
    }

    if (!isNamedConstLexeme(idTok.lexeme)) {
        SEMerrorToken(idTok, "'" + idTok.lexeme + "' is not a valid constant name");
    }

    if (!expect(TK_OP, "expected '=' in const declaration", "=")) {
        synchronize();
        return;
    }

    postfix.push_back(idTok.lexeme);
    int value = parseExpression();
    postfix.push_back("=");
    postfix.push_back(";");

    if (!expect(TK_SEP, "expected ';' after const declaration", ";")) {
        synchronize();
        return;
    }

    if (!table.declareNamedConst(idTok.lexeme, value)) {
        SEMerrorToken(idTok, "redefinition of '" + idTok.lexeme + "'");
    }
}

void Analyzer::parseDeclaration() {
    Token intTok = advance(); // int

    while (true) {
        if (!check(TK_ID)) {
            SYNerrorHere("expected identifier in declaration");
            synchronize();
            return;
        }

        Token idTok = advance();

        if (idTok.line != intTok.line) {
            SEMerrorToken(idTok, "line break after 'int' is not allowed");
        }

        if (isNamedConstLexeme(idTok.lexeme)) {
            SEMerrorToken(idTok, "identifier with constant-style name must be declared as const");
        }

        if (!table.declareVariable(idTok.lexeme)) {
            SEMerrorToken(idTok, "redefinition of '" + idTok.lexeme + "'");
        }

        if (match(TK_OP, "=")) {
            postfix.push_back(idTok.lexeme);
            int value = parseExpression();
            postfix.push_back("=");
            postfix.push_back(";");
            table.assignValueToVariable(idTok.lexeme, value);
        }

        if (match(TK_SEP, ",")) {
            continue;
        }

        if (match(TK_SEP, ";")) {
            break;
        }

        SYNerrorHere("expected ',' or ';' in declaration");
        synchronize();
        return;
    }
}

void Analyzer::parseAssignment() {
    Token idTok = advance();

    if (!table.isDeclared(idTok.lexeme)) {
        SYNerrorToken(idTok, "identifier '" + idTok.lexeme + "' used before declaration");
    }

    if (table.isNamedConst(idTok.lexeme)) {
        SEMerrorToken(idTok, "cannot assign to named constant '" + idTok.lexeme + "'");
    }

    string op = "";
    if (match(TK_OP, "=")) {
        op = "=";
    }
    else if (match(TK_OP, "+=")) {
        op = "+=";
    }
    else if (match(TK_OP, "-=")) {
        op = "-=";
    }
    else if (match(TK_OP, "*=")) {
        op = "*=";
    }
    else if (match(TK_OP, "/=")) {
        op = "/=";
    }
    else if (match(TK_OP, "%=")) {
        op = "%=";
    }
    else {
        SYNerrorHere("expected assignment operator (=, +=, -=, *=, /=, %=)");
        synchronize();
        return;
    }

    int currentValue = 0;
    if (op != "=") {
        if (!table.getIntValue(idTok.lexeme, currentValue)) {
            SEMerrorToken(idTok, "variable '" + idTok.lexeme + "' used before initialization");
            currentValue = 0;
        }
    }

    postfix.push_back(idTok.lexeme);
    int rightValue = parseExpression();

    int newValue = rightValue;
    if (op == "+=") {
        newValue = currentValue + rightValue;
        postfix.push_back("+");
    }
    else if (op == "-=") {
        newValue = currentValue - rightValue;
        postfix.push_back("-");
    }
    else if (op == "*=") {
        newValue = currentValue * rightValue;
        postfix.push_back("*");
    }
    else if (op == "/=") {
        if (rightValue == 0) {
            SEMerrorToken(tokens[pos - 1], "division by zero in assignment");
            newValue = 0;
        }
        else {
            newValue = currentValue / rightValue;
        }
        postfix.push_back("/");
    }
    else if (op == "%=") {
        if (rightValue == 0) {
            SEMerrorToken(tokens[pos - 1], "modulo by zero in assignment");
            newValue = 0;
        }
        else {
            newValue = currentValue % rightValue;
        }
        postfix.push_back("%");
    }

    postfix.push_back("=");
    postfix.push_back(";");

    if (!expect(TK_SEP, "expected ';' after assignment", ";")) {
        synchronize();
        return;
    }

    table.assignValueToVariable(idTok.lexeme, newValue);
}

void Analyzer::parseStatement() {
    if (check(TK_KW, "const")) {
        parseConstDeclaration();
        return;
    }

    if (check(TK_KW, "int")) {
        parseDeclaration();
        return;
    }

    if (check(TK_ID)) {
        parseAssignment();
        return;
    }

    SYNerrorHere("unexpected token '" + current().lexeme + "'");
    synchronize();
}

Analyzer::Analyzer(const vector<Token>& toks, MutableTable& tbl, ofstream& e)
    : tokens(toks), pos(0), table(tbl), err(e) {
}

void Analyzer::analyzeProgram() {
    while (!isAtEnd()) {
        parseStatement();
    }
}

void analyze(const string& datFile, const string& semErrFile,
    StaticTable& kw, StaticTable& op, StaticTable& sep,
    MutableTable& table){
    vector<Token> tokens = readTokensFromData(datFile, kw, op, sep, table);

    ofstream err(semErrFile);
    Analyzer analyzer(tokens, table, err);
    analyzer.analyzeProgram();
}


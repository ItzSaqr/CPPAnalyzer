// Подмножество языка С++ включает:
// данные типа int;
// инструкции описания переменных;
// операторы присваивания в любой последовательности;
// полный набор арифметических, логических операций и операций сравнения.

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cctype>
#include <algorithm>
#include <cstdio>
#include "analyzer.h"
#include "tables.h"
using namespace std;

vector<string> postfix;

const char* kindName(TokenKind k) {
    switch (k) {
    case TK_ID:  return "TK_ID";
    case TK_KW:  return "TK_KW";
    case TK_CNST: return "TK_CNST";
    case TK_OP:  return "TK_OP";
    case TK_SEP: return "TK_SEP";
    case TK_ERR: return "TK_ERR";
    case TK_EOF: return "TK_EOF";
    default:     return "UNKNOWN";
    }
}

bool isIdentStart(char c) {
    return isalpha((unsigned char)c) || c == '_';
}

bool isIdentChar(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

bool isNamedConstLexeme(const string& s) {
    if (s.empty()) return false;

    if (!(isupper((unsigned char)s[0]) || s[0] == '_')) return false;

    for (char c : s) {
        if (!(isupper((unsigned char)c) ||
              isdigit((unsigned char)c) ||
              c == '_')) {
            return false;
        }
    }

    return true;
}

Token nextToken(std::ifstream& in,
    StaticTable& kw,
    StaticTable& op,
    StaticTable& sep,
    MutableTable& mutableTable,
    int& line, int& col) {
    // пропуск пробелов и комментариев
    while (true) {
        int p = in.peek();
        if (p == EOF) return { TK_EOF, -1, line, col, "" };

        char c = (char)p;

        if (c == '\n') {
            in.get();
            line++;
            col = 1;
            continue;
        }

        if (isspace((unsigned char)c)) {
            in.get();
            col++;
            continue;
        }

        if (c == '/') {
            in.get();
            col++;

            int p2 = in.peek();
            if (p2 == EOF) {
                in.unget();
                col--;
                break;
            }

            char c2 = (char)p2;

            // однострочный комментарий //
            if (c2 == '/') {
                in.get();
                col++;

                while (true) {
                    int p3 = in.peek();
                    if (p3 == EOF) return { TK_EOF, -1, line, col, "" };

                    char d = (char)p3;
                    if (d == '\n') break;

                    in.get();
                    col++;
                }
                continue;
            }

            // многострочный комментарий /* ... */
            if (c2 == '*') {
                in.get();
                col++;

                bool closed = false;

                while (true) {
                    int p3 = in.get();
                    if (p3 == EOF) {
                        return { TK_ERR, -1, line, col, "unclosed comment" };
                    }

                    char d = (char)p3;

                    if (d == '\n') {
                        line++;
                        col = 1;
                        continue;
                    }

                    col++;

                    if (d == '*') {
                        int p4 = in.peek();
                        if (p4 == '/') {
                            in.get();
                            col++;
                            closed = true;
                            break;
                        }
                    }
                }

                if (!closed) {
                    return { TK_ERR, -1, line, col, "unclosed comment" };
                }

                continue;
            }

            // это не комментарий, а обычный '/'
            in.unget();
            col--;
        }

        break;
    }

    int startCol = col;
    char c;
    in.get(c); col++;

    // identifier / keyword / named const
    if (isIdentStart(c)) {
        string lex;
        lex += c;

        while (true) {
            int p = in.peek();
            if (p == EOF) break;
            char d = (char)p;
            if (!isIdentChar(d)) break;
            in.get(d); col++;
            lex += d;
        }

        int id = kw.find(lex);
        if (id != -1) return { TK_KW, id, line, startCol, lex };
        else {
            id = mutableTable.findOrAddIdentifier(lex);
            return { TK_ID, id, line, startCol, lex };
        }
    }

    // number
    if (isdigit((unsigned char)c)) {
        string num;
        num += c;

        while (true) {
            int p = in.peek();
            if (p == EOF) break;
            char d = (char)p;
            if (!std::isdigit((unsigned char)d)) break;
            in.get(d); col++;
            num += d;
        }

        int p = in.peek();
        if (isIdentStart(p)) {
            while (true) {
                int p = in.peek();
                if (p == EOF) break;
                char d = (char)p;
                if (!isIdentChar(d)) break;
                in.get(d); col++;
                num += d;
            }
            return{ TK_ERR, -1, line, startCol , num};
        }

        int val = 0;
        try { val = stoi(num); }
        catch (...) { return { TK_ERR, -1, line, startCol }; }

        int id = mutableTable.findOrAddConst(num, val);
        return { TK_CNST, id, line, startCol, num };
    }

    // operator: пробуем 2 символа, потом 1
    {
        int p = in.peek();
        string op2;
        op2 += c;
        if (p != EOF) op2 += (char)p;

        int id = op.find(op2);
        if (id != -1) {
            char buf;
            in.get(buf); col++;
            return { TK_OP, id, line, startCol, op2 };
        }
        id = op.find(string(1, c));
        if (id != -1) return { TK_OP, id, line, startCol, string(1, c) };
    }

    // separator
    int id = sep.find(string(1, c));
    if (id != -1) return { TK_SEP, id, line, startCol, string(1, c) };

    // error
    return { TK_ERR, (unsigned char)c, line, startCol, string(1, c) };
}

string tokenLexeme(const Token& t,
    StaticTable& kw, StaticTable& op, StaticTable& sep,
    MutableTable& mutableTable)
{
    switch (t.kind) {
    case TK_KW:  return kw.get(t.index);
    case TK_OP:  return op.get(t.index);
    case TK_SEP: return sep.get(t.index);
    case TK_ID:  return mutableTable.get(t.index);
    case TK_CNST: return mutableTable.get(t.index);
    default:     return "";
    }
}

int scan(const string& input, const string& txtOut, const string& errOut, const string& datOut,
    StaticTable& kw, StaticTable& op, StaticTable& sep,
    MutableTable& mutableTable)
{
    ifstream in(input);
    ofstream txt(txtOut);
    ofstream dat(datOut);
    ofstream err(errOut);
    if (!txt || !err || !in || !dat) return 0;

    int line = 1, col = 1;
    int currentOutputLine = -1;

    while (true) {
        Token t = nextToken(in, kw, op, sep, mutableTable, line, col);
        if (t.kind == TK_EOF) break;

        if (t.kind == TK_ERR) {
            if (!t.lexeme.empty()) {
                err << "LEX_ERR token=" << t.lexeme
                    << " line=" << t.line
                    << " col=" << t.col << "\n";
            }
            else {
                err << "LEX_ERR char=" << (char)t.index
                    << " code=" << t.index
                    << " line=" << t.line
                    << " col=" << t.col << "\n";
            }
        }
        else {
            dat << (int)t.kind << ' ' << t.index << ' ' << t.line << ' ' << t.col << '\n';

            string lex = tokenLexeme(t, kw, op, sep, mutableTable);

            if (t.line != currentOutputLine) {
                if (currentOutputLine != -1) {
                    txt << '\n';
                }
                currentOutputLine = t.line;
                txt << t.line << " | ";
            }

            txt << kindName(t.kind)
                << "(idx=" << t.index
                << ",row=" << t.line
                << ",col=" << t.col;

            if (!lex.empty()) {
                txt << ",name=" << lex;
            }

            txt << ") ";
        }
    }

    if (currentOutputLine != -1) {
        txt << '\n';
    }

    return 1;
}

vector<Token> readTokensFromData(const string& datFile,
    StaticTable& kw, StaticTable& op, StaticTable& sep,
    MutableTable& table)
{
    vector<Token> tokens;
    ifstream in(datFile);

    int kind, index, line, col;
    while (in >> kind >> index >> line >> col) {
        Token t;
        t.kind = (TokenKind)kind;
        t.index = index;
        t.line = line;
        t.col = col;
        t.lexeme = "";

        switch (t.kind) {
        case TK_KW:   t.lexeme = kw.get(index); break;
        case TK_OP:   t.lexeme = op.get(index); break;
        case TK_SEP:  t.lexeme = sep.get(index); break;
        case TK_ID:
        case TK_CNST: t.lexeme = table.get(index); break;
        default: break;
        }

        tokens.push_back(t);
    }

    return tokens;
}

int main() {
    StaticTable kw = StaticTable("keywords.txt");
    StaticTable op = StaticTable("operators.txt");
    StaticTable sep = StaticTable("separators.txt");

    //StaticTable kw = generateKeywords();
    //StaticTable op = generateOperators();
    //StaticTable sep = generateSeparators();

    MutableTable table(60);

    int ok = scan("input.txt", "tokens.txt", "errors.txt", "data.txt", kw, op, sep, table);
    cout << "scan: " << ok << "\n";

    cout << "mutable table:\n";
    table.print();

    ifstream lexErr("errors.txt");
    ifstream semErr("semantic_errors.txt");
    bool hasLexErrors = lexErr.peek() != EOF;
    lexErr.close();

    if (!hasLexErrors) {
        postfix.clear();
        analyze("data.txt", "semantic_errors.txt", kw, op, sep, table);
        cout << "\nafter semantic analysis:\n";
        table.print();

        bool hasSymErrors = semErr.peek() != EOF;

        if (!hasSymErrors) {
            cout << "\npostfix:\n";
            for (const string& s : postfix) {
                if (s == ";") cout << "\n";
                else cout << s << ' ';
            }
            cout << "\n";
        }
        else cout << "\npostfix doesnt show because syntax/lexical errors were found\n";
    }
    else {
        cout << "\nsemantic analysis skipped because syntax/lexical errors were found\n";
    }

    //kw.print();
    //op.print();
    //sep.print();

    return 0;
}

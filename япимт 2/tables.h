#ifndef TABLES_H
#define TABLES_H

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstdio>

using namespace std;

enum LexemType {
    LT_UNDEF = 0,
    LT_INT = 1
};

enum SymbolCategory {
    SC_UNDEF = 0,
    SC_VARIABLE = 1,
    SC_INT_LITERAL = 2,
    SC_NAMED_CONST = 3
};

class tableElem {
public:
    string lexem;
    bool used;
    bool deleted;

    int category;

    bool declared;
    bool initialized;

    bool hasType;
    int type;

    bool hasValue;
    int intValue;

    tableElem();
};

class StaticTable {
private:
    vector<string> table;
public:
    StaticTable();
    StaticTable(const vector<string>& _table);
    StaticTable(const string& filename);

    void print() const;
    int find(const string& lexem) const;
    string get(int idx) const;
};

StaticTable generateSeparators();
StaticTable generateKeywords();
StaticTable generateOperators();

class MutableTable {
private:
    vector<tableElem> table;
    int capacity;
    int usedCount;

    unsigned int hashString(const string& s) const;
    double loadFactor() const;
    int findExisting(const string& lexem) const;
    int findInsertPos(const string& lexem) const;
    void rehash(int newCapacity);
    void ensureCapacity();

public:
    MutableTable();
    MutableTable(int startCapacity);

    int findLexem(const string& lexem) const;
    int findOrAddIdentifier(const string& lexem);
    int findOrAddConst(const string& lexem, int val);

    bool declareVariable(const string& lexem);
    bool declareNamedConst(const string& lexem, int value);
    bool assignValueToVariable(const string& lexem, int value);

    bool isAnyConst(const string& lexem) const;
    bool isLiteralConst(const string& lexem) const;
    bool isNamedConst(const string& lexem) const;
    bool isVariable(const string& lexem) const;
    bool isDeclared(const string& lexem) const;

    bool addOrSetType(const string& lexem, int t);
    bool addOrSetIntValue(const string& lexem, int val);
    bool getType(const string& lexem, int& outType) const;
    bool getIntValue(const string& lexem, int& outVal) const;
    bool setInitialized(const string& lexem);
    int getCategory(const string& lexem) const;

    void print() const;
    string get(int idx) const;
};

#endif
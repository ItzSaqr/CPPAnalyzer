#include "tables.h"
#include <iostream>

using namespace std;

tableElem::tableElem() {
    lexem = "";
    used = false;
    deleted = false;

    category = SC_UNDEF;

    declared = false;
    initialized = false;

    hasType = false;
    type = LT_UNDEF;

    hasValue = false;
    intValue = 0;
}

// ==================== StaticTable ====================

StaticTable::StaticTable() {}

StaticTable::StaticTable(const vector<string>& _table) {
    table = _table;
    sort(table.begin(), table.end());
}

StaticTable::StaticTable(const string& filename) {
    fstream in(filename);
    if (!in.is_open()) {
        cout << "Cannot open file: " << filename << endl;
        return;
    }

    vector<string> vec;
    string buf;

    while (in >> buf) {
        vec.push_back(buf);
    }

    table = vec;
    sort(table.begin(), table.end());
}

void StaticTable::print() const {
    for (int i = 0; i < (int)table.size(); i++) {
        printf("index %d lexem %s\n", i, table[i].c_str());
    }
}

int StaticTable::find(const string& lexem) const {
    auto it = lower_bound(table.begin(), table.end(), lexem);
    if (it != table.end() && *it == lexem) {
        return (int)distance(table.begin(), it);
    }
    return -1;
}

string StaticTable::get(int idx) const {
    if (idx < 0 || idx >= (int)table.size()) return "";
    return table[idx];
}

StaticTable generateSeparators() {
    vector<string> separators = { ";", ",", "(", ")", "{", "}" };
    sort(separators.begin(), separators.end());
    return StaticTable(separators);
}

StaticTable generateKeywords() {
    vector<string> keywords = { "int", "const" };
    sort(keywords.begin(), keywords.end());
    return StaticTable(keywords);
}

StaticTable generateOperators() {
    vector<string> operators = {
        "+", "-", "*", "/", "%", "++", "--",
        "&&", "||", "!",
        "==", "!=", "<", ">", "<=", ">=",
        "=", "+=", "-=", "*=", "/=", "%="
    };
    sort(operators.begin(), operators.end());
    return StaticTable(operators);
}

// ==================== MutableTable ====================

unsigned int MutableTable::hashString(const string& s) const {
    unsigned int h = 5381u;
    for (int i = 0; i < (int)s.size(); i++) {
        h = h * 33 + (unsigned int)(unsigned char)s[i];
    }
    return h;
}

double MutableTable::loadFactor() const {
    if (capacity == 0) return 1.0;
    return (double)usedCount / (double)capacity;
}

int MutableTable::findExisting(const string& lexem) const {
    if (capacity == 0) return -1;

    unsigned int h = hashString(lexem);
    int start = (int)(h % (unsigned int)capacity);

    for (int step = 0; step < capacity; step++) {
        int i = (start + step) % capacity;

        if (!table[i].used) {
            if (!table[i].deleted) return -1;
        }
        else {
            if (table[i].lexem == lexem) return i;
        }
    }

    return -1;
}

int MutableTable::findInsertPos(const string& lexem) const {
    if (capacity == 0) return -1;

    unsigned int h = hashString(lexem);
    int start = (int)(h % (unsigned int)capacity);

    int firstDeleted = -1;

    for (int step = 0; step < capacity; step++) {
        int i = (start + step) % capacity;

        if (!table[i].used) {
            if (table[i].deleted) {
                if (firstDeleted == -1) firstDeleted = i;
                continue;
            }

            if (firstDeleted != -1) return firstDeleted;
            return i;
        }

        if (table[i].lexem == lexem) return i;
    }

    if (firstDeleted != -1) return firstDeleted;
    return -1;
}

void MutableTable::rehash(int newCapacity) {
    vector<tableElem> oldTable = table;

    table.clear();
    table.resize(newCapacity);
    capacity = newCapacity;
    usedCount = 0;

    for (int i = 0; i < (int)oldTable.size(); i++) {
        if (oldTable[i].used && !oldTable[i].deleted) {
            int pos = findInsertPos(oldTable[i].lexem);
            if (pos == -1) continue;

            table[pos] = oldTable[i];
            table[pos].used = true;
            table[pos].deleted = false;
            usedCount++;
        }
    }
}

void MutableTable::ensureCapacity() {
    if (capacity == 0) {
        capacity = 16;
        table.resize(capacity);
        usedCount = 0;
        return;
    }

    if (loadFactor() > 0.7) {
        rehash(capacity * 2);
    }
}

MutableTable::MutableTable() {
    capacity = 0;
    usedCount = 0;
}

MutableTable::MutableTable(int startCapacity) {
    capacity = startCapacity;
    if (capacity < 4) capacity = 4;
    table.resize(capacity);
    usedCount = 0;
}

int MutableTable::findLexem(const string& lexem) const {
    return findExisting(lexem);
}

int MutableTable::findOrAddIdentifier(const string& lexem) {
    ensureCapacity();

    int existing = findExisting(lexem);
    if (existing != -1) return existing;

    int pos = findInsertPos(lexem);
    if (pos == -1) return -1;

    table[pos] = tableElem();
    table[pos].lexem = lexem;
    table[pos].used = true;
    table[pos].deleted = false;
    table[pos].category = SC_VARIABLE;

    usedCount++;
    return pos;
}

int MutableTable::findOrAddConst(const string& lexem, int val) {
    ensureCapacity();

    int existing = findExisting(lexem);
    if (existing != -1) return existing;

    int pos = findInsertPos(lexem);
    if (pos == -1) return -1;

    table[pos] = tableElem();
    table[pos].lexem = lexem;
    table[pos].used = true;
    table[pos].deleted = false;
    table[pos].category = SC_INT_LITERAL;
    table[pos].hasValue = true;
    table[pos].intValue = val;
    table[pos].hasType = true;
    table[pos].type = LT_INT;

    usedCount++;
    return pos;
}

bool MutableTable::declareVariable(const string& lexem) {
    int idx = findOrAddIdentifier(lexem);
    if (idx == -1) return false;
    if (table[idx].declared) return false;

    table[idx].declared = true;
    table[idx].category = SC_VARIABLE;
    table[idx].hasType = true;
    table[idx].type = LT_INT;
    return true;
}

bool MutableTable::declareNamedConst(const string& lexem, int value) {
    int idx = findOrAddIdentifier(lexem);
    if (idx == -1) return false;
    if (table[idx].declared) return false;

    table[idx].declared = true;
    table[idx].initialized = true;
    table[idx].category = SC_NAMED_CONST;
    table[idx].hasType = true;
    table[idx].type = LT_INT;
    table[idx].hasValue = true;
    table[idx].intValue = value;
    return true;
}

bool MutableTable::assignValueToVariable(const string& lexem, int value) {
    int idx = findExisting(lexem);
    if (idx == -1) return false;
    if (!table[idx].declared) return false;
    if (table[idx].category == SC_NAMED_CONST) return false;

    table[idx].hasType = true;
    table[idx].type = LT_INT;
    table[idx].hasValue = true;
    table[idx].intValue = value;
    table[idx].initialized = true;
    return true;
}

bool MutableTable::isAnyConst(const string& lexem) const {
    int idx = findExisting(lexem);
    if (idx == -1) return false;

    return table[idx].category == SC_INT_LITERAL ||
        table[idx].category == SC_NAMED_CONST;
}

bool MutableTable::isLiteralConst(const string& lexem) const {
    int idx = findExisting(lexem);
    if (idx == -1) return false;

    return table[idx].category == SC_INT_LITERAL;
}

bool MutableTable::isNamedConst(const string& lexem) const {
    int idx = findExisting(lexem);
    if (idx == -1) return false;

    return table[idx].category == SC_NAMED_CONST;
}

bool MutableTable::isVariable(const string& lexem) const {
    int idx = findExisting(lexem);
    if (idx == -1) return false;

    return table[idx].category == SC_VARIABLE;
}

bool MutableTable::isDeclared(const string& lexem) const {
    int idx = findExisting(lexem);
    if (idx == -1) return false;
    return table[idx].declared;
}

bool MutableTable::addOrSetType(const string& lexem, int t) {
    int idx = findOrAddIdentifier(lexem);
    if (idx == -1) return false;
    if (isAnyConst(lexem)) return false;

    table[idx].hasType = true;
    table[idx].type = t;
    return true;
}

bool MutableTable::addOrSetIntValue(const string& lexem, int val) {
    int idx = findOrAddIdentifier(lexem);
    if (idx == -1) return false;
    if (isAnyConst(lexem)) return false;

    table[idx].hasType = true;
    table[idx].type = LT_INT;
    table[idx].hasValue = true;
    table[idx].intValue = val;
    return true;
}

bool MutableTable::getType(const string& lexem, int& outType) const {
    int idx = findLexem(lexem);
    if (idx == -1) return false;
    if (!table[idx].hasType) return false;

    outType = table[idx].type;
    return true;
}

bool MutableTable::getIntValue(const string& lexem, int& outVal) const {
    int idx = findLexem(lexem);
    if (idx == -1) return false;
    if (!table[idx].hasValue) return false;

    outVal = table[idx].intValue;
    return true;
}

bool MutableTable::setInitialized(const string& lexem) {
    int idx = findExisting(lexem);
    if (idx == -1) return false;

    table[idx].initialized = true;
    return true;
}

int MutableTable::getCategory(const string& lexem) const {
    int idx = findExisting(lexem);
    if (idx == -1) return SC_UNDEF;
    return table[idx].category;
}

void MutableTable::print() const {
    printf("capacity %d used %d\n", capacity, usedCount);

    for (int i = 0; i < capacity; i++) {
        if (table[i].used && !table[i].deleted) {
            printf("index %d lexem %s", i, table[i].lexem.c_str());

            if (table[i].hasType) {
                printf(" type %d", table[i].type);
            }

            printf(" category %d", table[i].category);

            if (table[i].hasValue) {
                printf(" value %d", table[i].intValue);
            }

            printf("\n");
        }
    }
}

string MutableTable::get(int idx) const {
    if (idx < 0 || idx >= capacity) return "";
    if (!table[idx].used || table[idx].deleted) return "";
    return table[idx].lexem;
}
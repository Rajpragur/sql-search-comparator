#pragma once
#include "table.h"
#include <map>
#include <string>

using namespace std;

class Database {
private:
    map<string, Table*> tables;

public:
    Database();
    ~Database();

    bool createTable(string name, vector<ColumnDef> columns);
    bool dropTable(string name);
    Table* getTable(string name);
    void showTables() const;
};

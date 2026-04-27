#include "database.h"
#include <iostream>

Database::Database() {
    tables["books"] = new Table("books", {
        {"accession_no", DataType::INT},
        {"title", DataType::STRING},
        {"author", DataType::STRING}
    });

    tables["books"]->insertRecord({"123", "DBMS", "Korth"});
    tables["books"]->insertRecord({"124", "Operating Systems", "Silberschatz"});
    tables["books"]->insertRecord({"125", "Computer Networks", "Tanenbaum"});
    tables["books"]->insertRecord({"126", "Clean Code", "Martin"});
}

Database::~Database() {
    for (auto const& [name, table] : tables) {
        delete table;
    }
}

bool Database::createTable(string name, vector<ColumnDef> columns) {
    if (tables.find(name) != tables.end()) {
        cout << "Error: Table '" << name << "' already exists.\n";
        return false;
    }
    tables[name] = new Table(name, columns);
    return true;
}

bool Database::dropTable(string name) {
    auto it = tables.find(name);
    if (it == tables.end()) {
        cout << "Error: Table '" << name << "' does not exist.\n";
        return false;
    }

    if (name == "books") {
        cout << "Error: Table 'books' is protected and cannot be dropped.\n";
        return false;
    }

    delete it->second;
    tables.erase(it);
    return true;
}

Table* Database::getTable(string name) {
    if (tables.find(name) == tables.end()) {
        return nullptr;
    }
    return tables[name];
}

void Database::showTables() const {
    if (tables.empty()) {
        cout << "No tables found in the database.\n";
        return;
    }
    cout << "Tables in the database:\n";
    for (auto const& [name, table] : tables) {
        cout << "- " << name << "\n";
    }
}

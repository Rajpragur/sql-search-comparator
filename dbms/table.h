#pragma once
#include "query.h"
#include <string>
#include <vector>

using namespace std;

struct TableSearchResult {
    bool success = true;
    string error;
    vector<string> columns;
    vector<vector<string>> rows;
    SearchMetrics metrics;
    map<string, double> benchmark;
    vector<TraceEvent> trace;
    map<string, AlgorithmAccess> algorithmAccess;
};

class Table {
private:
    string name;
    vector<ColumnDef> columns;
    vector<vector<string>> records;

    int findColumnIndex(const string& columnName) const;
    TableSearchResult executeSearch(
        const string& columnName,
        const string& value,
        SearchAlgorithm algorithm,
        const string& algorithmLabel,
        const string& traceStage,
        long long* sequenceCounter,
        bool captureObservability
    ) const;

public:
    Table(string name, vector<ColumnDef> columns);

    string getName() const;
    const vector<ColumnDef>& getColumns() const;
    vector<string> getColumnNames() const;

    // Returns true if insertion is successful (type validation passes)
    bool insertRecord(const vector<string>& record);

    void printAllRecords() const;
    TableSearchResult selectRecords(const string& columnName, const string& value, SearchAlgorithm algorithm, bool benchmark) const;

    // Snapshot methods for transaction rollbacks
    vector<vector<string>> getRecords() const;
    void setRecords(const vector<vector<string>>& newRecords);
};

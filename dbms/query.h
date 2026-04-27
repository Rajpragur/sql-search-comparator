#pragma once
#include <map>
#include <string>
#include <vector>

using namespace std;

enum class DataType { INT, STRING };

struct ColumnDef {
    string name;
    DataType type;
};

enum class QueryType { CREATE, INSERT, SELECT, DROP, BEGIN_TXN, COMMIT_TXN, ABORT_TXN, UNKNOWN };
enum class SearchAlgorithm { LINEAR, BINARY, BTREE, HASH };

struct SearchMetrics {
    double executionTimeMs = 0.0;
    size_t comparisons = 0;
};

struct TraceEvent {
    long long id = 0;
    string stage;
    string type;
    string message;
    string tableName;
    string algorithm;
    string lockType;
    string key;
    string value;
    string status;
    int txnId = -1;
    int rowIndex = -1;
    bool matched = false;
    map<string, string> metadata;
};

struct RowTouch {
    int rowIndex = -1;
    int order = 0;
    bool matched = false;
    string comparedValue;
    string detail;
};

struct AlgorithmAccess {
    vector<RowTouch> touches;
    vector<int> matchedRows;
    vector<int> sortedOrder;
    string detail;
};

struct LockSnapshot {
    string tableName;
    int exclusiveHolder = -1;
    vector<int> sharedHolders;
    vector<int> waitingShared;
    vector<int> waitingExclusive;
};

struct WaitEdge {
    int fromTxnId = -1;
    int toTxnId = -1;
    string tableName;
    string requestedLockType;
};

// Represents a parsed query
struct Query {
    QueryType type = QueryType::UNKNOWN;
    string tableName;
    vector<ColumnDef> columns;    // Used for CREATE operation
    vector<string> values;        // Used for INSERT operation
    string whereColumn;           // Used for SELECT filtering
    string whereValue;            // Used for SELECT filtering
    SearchAlgorithm searchAlgorithm = SearchAlgorithm::LINEAR;
    bool benchmark = false;
};

struct QueryExecutionResult {
    bool success = true;
    string message;
    vector<string> columns;
    vector<vector<string>> rows;
    string searchAlgorithm;
    SearchMetrics metrics;
    map<string, double> benchmark;
    string logs;
    vector<TraceEvent> trace;
    map<string, AlgorithmAccess> algorithmAccess;
    vector<LockSnapshot> lockSnapshots;
    vector<WaitEdge> waitForGraph;
    bool deadlockRisk = false;
};

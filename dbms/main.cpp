#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "database.h"
#include "executor.h"
#include "lock_manager.h"
#include "parser.h"
#include "transaction.h"

using namespace std;

namespace {
string algorithmName(SearchAlgorithm algorithm) {
    switch (algorithm) {
        case SearchAlgorithm::LINEAR:
            return "Linear";
        case SearchAlgorithm::BINARY:
            return "Binary";
        case SearchAlgorithm::BTREE:
            return "BTree";
        case SearchAlgorithm::HASH:
            return "Hash";
    }
    return "Linear";
}

string queryTypeName(QueryType type) {
    switch (type) {
        case QueryType::CREATE:
            return "CREATE";
        case QueryType::INSERT:
            return "INSERT";
        case QueryType::SELECT:
            return "SELECT";
        case QueryType::DROP:
            return "DROP";
        case QueryType::BEGIN_TXN:
            return "BEGIN";
        case QueryType::COMMIT_TXN:
            return "COMMIT";
        case QueryType::ABORT_TXN:
            return "ABORT";
        case QueryType::UNKNOWN:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

string trim(const string& value) {
    size_t first = value.find_first_not_of(" \n\r\t");
    if (first == string::npos) {
        return "";
    }
    size_t last = value.find_last_not_of(" \n\r\t");
    return value.substr(first, last - first + 1);
}

string escapeJson(const string& value) {
    string escaped;
    for (char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += ch; break;
        }
    }
    return escaped;
}

void printJsonIntArray(const vector<int>& values) {
    cout << "[";
    for (size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            cout << ",";
        }
        cout << values[index];
    }
    cout << "]";
}

void printJsonResult(const QueryExecutionResult& result) {
    streamsize previousPrecision = cout.precision();
    ios::fmtflags previousFlags = cout.flags();
    cout << fixed << setprecision(3);

    cout << "{";
    cout << "\"success\":" << (result.success ? "true" : "false") << ",";
    cout << "\"message\":\"" << escapeJson(result.message) << "\",";
    cout << "\"logs\":\"" << escapeJson(result.logs) << "\",";
    cout << "\"results\":[";
    for (size_t rowIndex = 0; rowIndex < result.rows.size(); ++rowIndex) {
        if (rowIndex > 0) {
            cout << ",";
        }
        cout << "{";
        for (size_t colIndex = 0; colIndex < result.columns.size(); ++colIndex) {
            if (colIndex > 0) {
                cout << ",";
            }
            string cellValue = colIndex < result.rows[rowIndex].size() ? result.rows[rowIndex][colIndex] : "";
            cout << "\"" << escapeJson(result.columns[colIndex]) << "\":\"" << escapeJson(cellValue) << "\"";
        }
        cout << "}";
    }
    cout << "],";
    cout << "\"search_algorithm\":\"" << escapeJson(result.searchAlgorithm) << "\",";
    cout << "\"execution_time_ms\":" << result.metrics.executionTimeMs << ",";
    cout << "\"comparisons\":" << result.metrics.comparisons << ",";
    cout << "\"benchmark\":{";
    bool firstEntry = true;
    for (const auto& [name, timing] : result.benchmark) {
        if (!firstEntry) {
            cout << ",";
        }
        firstEntry = false;
        cout << "\"" << escapeJson(name) << "\":" << timing;
    }
    cout << "},";
    cout << "\"trace\":[";
    for (size_t traceIndex = 0; traceIndex < result.trace.size(); ++traceIndex) {
        if (traceIndex > 0) {
            cout << ",";
        }
        const TraceEvent& event = result.trace[traceIndex];
        cout << "{";
        cout << "\"id\":" << event.id << ",";
        cout << "\"stage\":\"" << escapeJson(event.stage) << "\",";
        cout << "\"type\":\"" << escapeJson(event.type) << "\",";
        cout << "\"message\":\"" << escapeJson(event.message) << "\",";
        cout << "\"table_name\":\"" << escapeJson(event.tableName) << "\",";
        cout << "\"algorithm\":\"" << escapeJson(event.algorithm) << "\",";
        cout << "\"lock_type\":\"" << escapeJson(event.lockType) << "\",";
        cout << "\"key\":\"" << escapeJson(event.key) << "\",";
        cout << "\"value\":\"" << escapeJson(event.value) << "\",";
        cout << "\"status\":\"" << escapeJson(event.status) << "\",";
        cout << "\"txn_id\":" << event.txnId << ",";
        cout << "\"row_index\":" << event.rowIndex << ",";
        cout << "\"matched\":" << (event.matched ? "true" : "false") << ",";
        cout << "\"metadata\":{";
        bool firstMeta = true;
        for (const auto& [metaKey, metaValue] : event.metadata) {
            if (!firstMeta) {
                cout << ",";
            }
            firstMeta = false;
            cout << "\"" << escapeJson(metaKey) << "\":\"" << escapeJson(metaValue) << "\"";
        }
        cout << "}";
        cout << "}";
    }
    cout << "],";
    cout << "\"algorithm_access\":{";
    bool firstAccess = true;
    for (const auto& [name, access] : result.algorithmAccess) {
        if (!firstAccess) {
            cout << ",";
        }
        firstAccess = false;
        cout << "\"" << escapeJson(name) << "\":{";
        cout << "\"detail\":\"" << escapeJson(access.detail) << "\",";
        cout << "\"matched_rows\":";
        printJsonIntArray(access.matchedRows);
        cout << ",";
        cout << "\"sorted_order\":";
        printJsonIntArray(access.sortedOrder);
        cout << ",";
        cout << "\"touches\":[";
        for (size_t touchIndex = 0; touchIndex < access.touches.size(); ++touchIndex) {
            if (touchIndex > 0) {
                cout << ",";
            }
            const RowTouch& touch = access.touches[touchIndex];
            cout << "{";
            cout << "\"row_index\":" << touch.rowIndex << ",";
            cout << "\"order\":" << touch.order << ",";
            cout << "\"matched\":" << (touch.matched ? "true" : "false") << ",";
            cout << "\"compared_value\":\"" << escapeJson(touch.comparedValue) << "\",";
            cout << "\"detail\":\"" << escapeJson(touch.detail) << "\"";
            cout << "}";
        }
        cout << "]";
        cout << "}";
    }
    cout << "},";
    cout << "\"lock_snapshots\":[";
    for (size_t snapshotIndex = 0; snapshotIndex < result.lockSnapshots.size(); ++snapshotIndex) {
        if (snapshotIndex > 0) {
            cout << ",";
        }
        const LockSnapshot& snapshot = result.lockSnapshots[snapshotIndex];
        cout << "{";
        cout << "\"table_name\":\"" << escapeJson(snapshot.tableName) << "\",";
        cout << "\"exclusive_holder\":" << snapshot.exclusiveHolder << ",";
        cout << "\"shared_holders\":";
        printJsonIntArray(snapshot.sharedHolders);
        cout << ",";
        cout << "\"waiting_shared\":";
        printJsonIntArray(snapshot.waitingShared);
        cout << ",";
        cout << "\"waiting_exclusive\":";
        printJsonIntArray(snapshot.waitingExclusive);
        cout << "}";
    }
    cout << "],";
    cout << "\"wait_for_graph\":[";
    for (size_t edgeIndex = 0; edgeIndex < result.waitForGraph.size(); ++edgeIndex) {
        if (edgeIndex > 0) {
            cout << ",";
        }
        const WaitEdge& edge = result.waitForGraph[edgeIndex];
        cout << "{";
        cout << "\"from_txn_id\":" << edge.fromTxnId << ",";
        cout << "\"to_txn_id\":" << edge.toTxnId << ",";
        cout << "\"table_name\":\"" << escapeJson(edge.tableName) << "\",";
        cout << "\"requested_lock_type\":\"" << escapeJson(edge.requestedLockType) << "\"";
        cout << "}";
    }
    cout << "],";
    cout << "\"deadlock_risk\":" << (result.deadlockRisk ? "true" : "false");
    cout << "}";

    cout.flags(previousFlags);
    cout.precision(previousPrecision);
}

QueryExecutionResult runQuerySequence(const string& rawQueries, Database& db, TransactionManager& tm, LockManager& lm, bool benchmark) {
    Executor executor(&db, &tm, &lm);
    istringstream tokenStream(rawQueries);
    string queryStr;
    QueryExecutionResult lastResult;
    bool executedAtLeastOne = false;
    long long parseSequence = 1;
    vector<TraceEvent> aggregatedTrace;

    while (getline(tokenStream, queryStr, ';')) {
        string cleanQuery = trim(queryStr);
        if (cleanQuery.empty()) {
            continue;
        }

        Query parsedQuery = Parser::parse(cleanQuery);
        parsedQuery.benchmark = benchmark && tokenStream.rdbuf()->in_avail() == 0;
        lastResult = executor.execute(parsedQuery);
        executedAtLeastOne = true;

        TraceEvent parseEvent;
        parseEvent.id = parseSequence++;
        parseEvent.stage = "parse";
        parseEvent.type = "statement";
        parseEvent.message = "Parsed statement for execution.";
        parseEvent.tableName = parsedQuery.tableName;
        parseEvent.algorithm = parsedQuery.type == QueryType::SELECT ? algorithmName(parsedQuery.searchAlgorithm) : "";
        parseEvent.metadata = {
            {"query_type", queryTypeName(parsedQuery.type)},
            {"benchmark", parsedQuery.benchmark ? "true" : "false"},
            {"where_column", parsedQuery.whereColumn},
            {"where_value", parsedQuery.whereValue}
        };
        aggregatedTrace.push_back(parseEvent);

        for (auto event : lastResult.trace) {
            event.id = parseSequence++;
            aggregatedTrace.push_back(event);
        }
        lastResult.trace = aggregatedTrace;

        if (!lastResult.success) {
            return lastResult;
        }
    }

    if (!executedAtLeastOne) {
        lastResult.success = false;
        lastResult.message = "Error: No executable query was provided.";
    } else if (lastResult.message.empty()) {
        lastResult.message = "Success: Query sequence executed.";
    }

    return lastResult;
}

void runQueries(const string& rawQueries, Database& db, TransactionManager& tm, LockManager& lm) {
    Executor executor(&db, &tm, &lm);
    istringstream tokenStream(rawQueries);
    string queryStr;

    while (getline(tokenStream, queryStr, ';')) {
        string cleanQuery = trim(queryStr);
        if (cleanQuery.empty()) {
            continue;
        }

        Query parsedQuery = Parser::parse(cleanQuery);
        executor.execute(parsedQuery);
    }
}
}

int main(int argc, char* argv[]) {
    Database db;
    TransactionManager tm;
    LockManager lm;

    if (argc > 2 && string(argv[1]) == "--json") {
        string queryText = argv[2];
        bool benchmark = false;
        for (int i = 3; i < argc; ++i) {
            if (string(argv[i]) == "--benchmark") {
                benchmark = true;
            }
        }

        ostringstream capturedLogs;
        streambuf* originalCout = cout.rdbuf(capturedLogs.rdbuf());
        QueryExecutionResult result = runQuerySequence(queryText, db, tm, lm, benchmark);
        cout.rdbuf(originalCout);

        result.logs = capturedLogs.str();
        if (result.message.empty()) {
            result.message = result.success ? "Success: Query executed." : "Error: Query execution failed.";
        }

        printJsonResult(result);
        return result.success ? 0 : 1;
    }

    if (argc > 1) {
        string passedArgs = argv[1];
        runQueries(passedArgs, db, tm, lm);
        return 0;
    }

    cout << "Welcome to Phase 3 - Search, Benchmark and 2PL Concurrency.\n";
    cout << "Type SQL commands (e.g. BEGIN, COMMIT, ABORT, SELECT, INSERT). Type 'exit' to quit.\n";

    Executor executor(&db, &tm, &lm);
    string input;

    while (true) {
        cout << "\ndb> ";
        getline(cin, input);

        if (input == "exit" || input == "quit") {
            break;
        }

        if (input.empty()) {
            continue;
        }

        Query parsedQuery = Parser::parse(input);
        executor.execute(parsedQuery);
    }

    return 0;
}

#include "executor.h"
#include <iostream>

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
}

Executor::Executor(Database* db, TransactionManager* tm, LockManager* lm)
    : db(db), tm(tm), lm(lm), currentTxnId(-1) {}

QueryExecutionResult Executor::execute(const Query& query) {
    QueryExecutionResult result;
    long long sequenceCounter = 1;
    lm->setTraceBuffer(&result.trace, &sequenceCounter);

    auto emitTrace = [&](const string& stage,
                         const string& type,
                         const string& message,
                         const string& tableName = "",
                         const string& algorithm = "",
                         int txnId = -1,
                         const map<string, string>& metadata = map<string, string>()) {
        TraceEvent event;
        event.id = sequenceCounter++;
        event.stage = stage;
        event.type = type;
        event.message = message;
        event.tableName = tableName;
        event.algorithm = algorithm;
        event.txnId = txnId;
        event.metadata = metadata;
        result.trace.push_back(event);
    };

    if (query.type == QueryType::UNKNOWN) {
        result.success = false;
        result.message = "Error: Unrecognized or malformed query.";
        emitTrace("parse", "error", "Query type could not be determined.");
        cout << result.message << "\n";
        lm->clearTraceBuffer();
        return result;
    }

    if (query.type == QueryType::BEGIN_TXN) {
        if (currentTxnId != -1) {
            result.success = false;
            result.message = "Error: Transaction already active.";
            emitTrace("transaction", "error", "BEGIN rejected because a transaction is already active.", "", "", currentTxnId);
            cout << result.message << "\n";
            lm->clearTraceBuffer();
            return result;
        }
        currentTxnId = tm->beginTransaction();
        result.message = "Transaction started.";
        emitTrace("transaction", "begin", "Transaction started.", "", "", currentTxnId);
        result.lockSnapshots = lm->getLockSnapshots();
        result.waitForGraph = lm->getWaitForGraph();
        result.deadlockRisk = lm->hasDeadlockRisk();
        lm->clearTraceBuffer();
        return result;
    }

    if (query.type == QueryType::COMMIT_TXN || query.type == QueryType::ABORT_TXN) {
        if (currentTxnId == -1) {
            result.success = false;
            result.message = "Error: No active transaction.";
            emitTrace("transaction", "error", "No active transaction to finish.");
            cout << result.message << "\n";
            lm->clearTraceBuffer();
            return result;
        }
        if (query.type == QueryType::COMMIT_TXN) {
            emitTrace("commit", "prepare", "Preparing to commit transaction.", "", "", currentTxnId);
            lm->releaseAllLocks(currentTxnId);
            tm->commitTransaction(currentTxnId);
            result.message = "Transaction committed.";
            emitTrace("commit", "complete", "Transaction committed.", "", "", currentTxnId);
        } else {
            emitTrace("abort", "prepare", "Preparing to abort transaction.", "", "", currentTxnId);
            Transaction* txn = tm->getTransaction(currentTxnId);
            if (txn) {
                for (auto const& [tableName, backupRecords] : txn->tableSnapshots) {
                    Table* table = db->getTable(tableName);
                    if (table) {
                        table->setRecords(backupRecords);
                        emitTrace("abort", "rollback", "Restored snapshot for table.", tableName, "", currentTxnId, {{"rows_restored", to_string(backupRecords.size())}});
                        cout << "Restored table '" << tableName << "' to pre-transaction state.\n";
                    }
                }
            }
            lm->releaseAllLocks(currentTxnId);
            tm->abortTransaction(currentTxnId);
            result.message = "Transaction aborted.";
            emitTrace("abort", "complete", "Transaction aborted.", "", "", currentTxnId);
        }
        currentTxnId = -1;
        result.lockSnapshots = lm->getLockSnapshots();
        result.waitForGraph = lm->getWaitForGraph();
        result.deadlockRisk = lm->hasDeadlockRisk();
        lm->clearTraceBuffer();
        return result;
    }

    bool autoCommit = false;
    if (currentTxnId == -1) {
        currentTxnId = tm->beginTransaction();
        autoCommit = true;
        emitTrace("transaction", "auto_begin", "Started an auto-commit transaction for this statement.", query.tableName, "", currentTxnId);
    } else {
        emitTrace("transaction", "reuse", "Executing inside the active transaction.", query.tableName, "", currentTxnId);
    }

    if (query.tableName.empty()) {
        result.success = false;
        result.message = "Error: Missing table name in query.";
        emitTrace("parse", "error", "Parsed query is missing a target table.", "", query.type == QueryType::SELECT ? algorithmName(query.searchAlgorithm) : "", currentTxnId);
        cout << result.message << "\n";
        if (autoCommit) {
            lm->releaseAllLocks(currentTxnId);
            tm->abortTransaction(currentTxnId);
            currentTxnId = -1;
        }
        result.lockSnapshots = lm->getLockSnapshots();
        result.waitForGraph = lm->getWaitForGraph();
        result.deadlockRisk = lm->hasDeadlockRisk();
        lm->clearTraceBuffer();
        return result;
    }

    emitTrace(
        "parse",
        "query_ready",
        "Prepared parsed query for execution.",
        query.tableName,
        query.type == QueryType::SELECT ? algorithmName(query.searchAlgorithm) : "",
        currentTxnId,
        {
            {"query_type", queryTypeName(query.type)},
            {"benchmark", query.benchmark ? "true" : "false"},
            {"where_column", query.whereColumn},
            {"where_value", query.whereValue}
        }
    );

    switch (query.type) {
        case QueryType::CREATE: {
            lm->acquireLock(currentTxnId, "CATALOG", LockType::EXCLUSIVE);
            if (db->createTable(query.tableName, query.columns)) {
                result.message = "Success: Table '" + query.tableName + "' created.";
                emitTrace("mutation", "create", "Created table.", query.tableName, "", currentTxnId, {{"column_count", to_string(query.columns.size())}});
                cout << result.message << "\n";
            } else {
                result.success = false;
                result.message = "Error: Failed to create table '" + query.tableName + "'.";
                emitTrace("mutation", "error", "Failed to create table.", query.tableName, "", currentTxnId);
            }
            break;
        }
        case QueryType::DROP: {
            lm->acquireLock(currentTxnId, "CATALOG", LockType::EXCLUSIVE);
            if (db->dropTable(query.tableName)) {
                result.message = "Success: Table '" + query.tableName + "' dropped.";
                emitTrace("mutation", "drop", "Dropped table.", query.tableName, "", currentTxnId);
                cout << result.message << "\n";
            } else {
                result.success = false;
                result.message = "Error: Failed to drop table '" + query.tableName + "'.";
                emitTrace("mutation", "error", "Failed to drop table.", query.tableName, "", currentTxnId);
            }
            break;
        }
        case QueryType::INSERT: {
            lm->acquireLock(currentTxnId, query.tableName, LockType::EXCLUSIVE);
            Table* table = db->getTable(query.tableName);
            if (!table) {
                result.success = false;
                result.message = "Error: Table '" + query.tableName + "' does not exist.";
                emitTrace("mutation", "error", "Insert target table does not exist.", query.tableName, "", currentTxnId);
                cout << result.message << "\n";
            } else {
                Transaction* txn = tm->getTransaction(currentTxnId);
                if (txn && txn->writeLockedTables.find(query.tableName) == txn->writeLockedTables.end()) {
                    txn->tableSnapshots[query.tableName] = table->getRecords();
                    txn->writeLockedTables.insert(query.tableName);
                    emitTrace("transaction", "snapshot", "Captured table snapshot before mutation.", query.tableName, "", currentTxnId, {{"rows", to_string(txn->tableSnapshots[query.tableName].size())}});
                }

                if (table->insertRecord(query.values)) {
                    result.message = "Success: 1 record inserted into '" + query.tableName + "'.";
                    emitTrace("mutation", "insert", "Inserted a record into table.", query.tableName, "", currentTxnId, {{"value_count", to_string(query.values.size())}});
                    cout << result.message << "\n";
                } else {
                    result.success = false;
                    result.message = "Error: Insert failed for table '" + query.tableName + "'.";
                    emitTrace("mutation", "error", "Insert failed for table.", query.tableName, "", currentTxnId);
                }
            }
            break;
        }
        case QueryType::SELECT: {
            lm->acquireLock(currentTxnId, query.tableName, LockType::SHARED);
            Table* table = db->getTable(query.tableName);
            if (!table) {
                result.success = false;
                result.message = "Error: Table '" + query.tableName + "' does not exist.";
                emitTrace("search", "error", "Select target table does not exist.", query.tableName, algorithmName(query.searchAlgorithm), currentTxnId);
                cout << result.message << "\n";
            } else {
                result.columns = table->getColumnNames();
                result.searchAlgorithm = algorithmName(query.searchAlgorithm);
                emitTrace("search", "start", "Starting SELECT execution.", query.tableName, result.searchAlgorithm, currentTxnId, {{"where_column", query.whereColumn}, {"where_value", query.whereValue}});

                if (query.whereColumn.empty()) {
                    result.rows = table->getRecords();
                    result.metrics.comparisons = result.rows.size();
                    result.message = result.rows.empty()
                        ? "Table '" + query.tableName + "' is empty."
                        : "Success: Records fetched from '" + query.tableName + "'.";
                    emitTrace("search", "scan_all", "Fetched all rows without a WHERE clause.", query.tableName, result.searchAlgorithm, currentTxnId, {{"rows", to_string(result.rows.size())}});
                    table->printAllRecords();
                } else {
                    TableSearchResult searchResult = table->selectRecords(
                        query.whereColumn,
                        query.whereValue,
                        query.searchAlgorithm,
                        query.benchmark
                    );

                    result.success = searchResult.success;
                    result.message = searchResult.success
                        ? "Success: Query executed."
                        : searchResult.error;
                    result.columns = searchResult.columns;
                    result.rows = searchResult.rows;
                    result.metrics = searchResult.metrics;
                    result.benchmark = searchResult.benchmark;
                    result.algorithmAccess = searchResult.algorithmAccess;
                    result.trace.insert(result.trace.end(), searchResult.trace.begin(), searchResult.trace.end());

                    if (searchResult.success) {
                        if (searchResult.rows.empty()) {
                            cout << "No matching rows found in '" << query.tableName << "'.\n";
                        } else {
                            for (const auto& col : result.columns) {
                                cout << col << " | ";
                            }
                            cout << "\n";
                            for (size_t i = 0; i < result.columns.size() * 18; ++i) {
                                cout << "-";
                            }
                            cout << "\n";
                            for (const auto& row : result.rows) {
                                for (const auto& value : row) {
                                    cout << value << " | ";
                                }
                                cout << "\n";
                            }
                        }
                    } else {
                        cout << result.message << "\n";
                    }
                }
            }
            break;
        }
        default:
            break;
    }

    if (autoCommit) {
        emitTrace("commit", "prepare", "Preparing auto-commit transaction finalization.", query.tableName, "", currentTxnId);
        lm->releaseAllLocks(currentTxnId);
        tm->commitTransaction(currentTxnId);
        emitTrace("commit", "complete", "Auto-commit transaction completed.", query.tableName, "", currentTxnId);
        currentTxnId = -1;
    }

    result.lockSnapshots = lm->getLockSnapshots();
    result.waitForGraph = lm->getWaitForGraph();
    result.deadlockRisk = lm->hasDeadlockRisk();
    lm->clearTraceBuffer();
    return result;
}

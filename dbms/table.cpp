#include "table.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <unordered_map>

using namespace std;

namespace {
long long toIntValue(const string& value) {
    return stoll(value);
}

bool isQuoted(const string& value) {
    return value.size() >= 2 &&
           ((value.front() == '\'' && value.back() == '\'') ||
            (value.front() == '"' && value.back() == '"'));
}

string normalizeValue(const string& value) {
    if (isQuoted(value)) {
        return value.substr(1, value.size() - 2);
    }
    return value;
}

bool isIntegerToken(const string& value) {
    string normalized = normalizeValue(value);
    if (normalized.empty()) {
        return false;
    }

    size_t start = normalized[0] == '-' ? 1 : 0;
    if (start == normalized.size()) {
        return false;
    }

    for (size_t i = start; i < normalized.size(); ++i) {
        if (!isdigit(static_cast<unsigned char>(normalized[i]))) {
            return false;
        }
    }
    return true;
}

string algorithmLabel(SearchAlgorithm algorithm) {
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

void pushTrace(
    vector<TraceEvent>& trace,
    long long* sequenceCounter,
    const string& stage,
    const string& type,
    const string& message,
    const string& algorithm,
    const string& key,
    const string& value,
    int rowIndex,
    bool matched,
    const map<string, string>& metadata = {}) {
    if (!sequenceCounter) {
        return;
    }

    TraceEvent event;
    event.id = (*sequenceCounter)++;
    event.stage = stage;
    event.type = type;
    event.message = message;
    event.algorithm = algorithm;
    event.key = key;
    event.value = value;
    event.rowIndex = rowIndex;
    event.matched = matched;
    event.metadata = metadata;
    trace.push_back(event);
}

void recordTouch(
    AlgorithmAccess& access,
    int rowIndex,
    int order,
    bool matched,
    const string& comparedValue,
    const string& detail) {
    access.touches.push_back({rowIndex, order, matched, comparedValue, detail});
    if (matched) {
        access.matchedRows.push_back(rowIndex);
    }
}
}

Table::Table(string name, vector<ColumnDef> columns) : name(name), columns(columns) {}

string Table::getName() const {
    return name;
}

const vector<ColumnDef>& Table::getColumns() const {
    return columns;
}

vector<string> Table::getColumnNames() const {
    vector<string> names;
    for (const auto& col : columns) {
        names.push_back(col.name);
    }
    return names;
}

int Table::findColumnIndex(const string& columnName) const {
    for (size_t i = 0; i < columns.size(); ++i) {
        if (columns[i].name == columnName) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool Table::insertRecord(const vector<string>& record) {
    if (columns.empty()) {
        for (size_t i = 0; i < record.size(); ++i) {
            columns.push_back({
                "col" + to_string(i + 1),
                isIntegerToken(record[i]) ? DataType::INT : DataType::STRING
            });
        }
    }

    if (record.size() != columns.size()) {
        cout << "Error: Column count doesn't match value count.\n";
        return false;
    }

    for (size_t i = 0; i < record.size(); ++i) {
        if (columns[i].type == DataType::INT) {
            string val = normalizeValue(record[i]);
            size_t start = 0;
            if (!val.empty() && val[0] == '-') {
                start = 1;
            }
            if (val.empty() || start == val.size()) {
                cout << "Error: Invalid integer format for column '" << columns[i].name << "'.\n";
                return false;
            }
            for (size_t j = start; j < val.size(); ++j) {
                if (!isdigit(static_cast<unsigned char>(val[j]))) {
                    cout << "Error: Invalid integer format for column '" << columns[i].name << "'.\n";
                    return false;
                }
            }
        }
    }

    vector<string> normalized = record;
    for (auto& value : normalized) {
        value = normalizeValue(value);
    }

    records.push_back(normalized);
    return true;
}

void Table::printAllRecords() const {
    if (records.empty()) {
        cout << "Table '" << name << "' is empty.\n";
        return;
    }

    for (const auto& col : columns) {
        cout << left << setw(15) << col.name + (col.type == DataType::INT ? "(INT)" : "(STR)") << " | ";
    }
    cout << "\n";
    for (size_t i = 0; i < columns.size() * 18; ++i) {
        cout << "-";
    }
    cout << "\n";

    for (const auto& row : records) {
        for (const auto& val : row) {
            cout << left << setw(15) << val << " | ";
        }
        cout << "\n";
    }
}

TableSearchResult Table::executeSearch(
    const string& columnName,
    const string& value,
    SearchAlgorithm algorithm,
    const string& algorithmName,
    const string& traceStage,
    long long* sequenceCounter,
    bool captureObservability) const {
    TableSearchResult result;
    result.columns = getColumnNames();

    const int columnIndex = findColumnIndex(columnName);
    if (columnIndex == -1) {
        result.success = false;
        result.error = "Error: Column '" + columnName + "' does not exist in table '" + name + "'.";
        return result;
    }

    const bool isIntColumn = columns[columnIndex].type == DataType::INT;
    const string normalizedValue = normalizeValue(value);
    long long targetInt = 0;

    if (isIntColumn) {
        try {
            targetInt = toIntValue(normalizedValue);
        } catch (const exception&) {
            result.success = false;
            result.error = "Error: Value '" + normalizedValue + "' is not a valid integer for column '" + columnName + "'.";
            return result;
        }
    }

    if (captureObservability) {
        pushTrace(
            result.trace,
            sequenceCounter,
            traceStage,
            "search_start",
            "Starting search execution.",
            algorithmName,
            columnName,
            normalizedValue,
            -1,
            false
        );
    }

    auto appendRow = [&](size_t recordIndex) {
        if (recordIndex < records.size()) {
            result.rows.push_back(records[recordIndex]);
        }
    };

    switch (algorithm) {
        case SearchAlgorithm::LINEAR: {
            for (size_t i = 0; i < records.size(); ++i) {
                result.metrics.comparisons++;
                const string& currentValue = records[i][columnIndex];
                bool isMatch = isIntColumn
                    ? toIntValue(currentValue) == targetInt
                    : currentValue == normalizedValue;
                if (captureObservability) {
                    recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(i), result.metrics.comparisons, isMatch, currentValue, "linear-compare");
                    pushTrace(
                        result.trace,
                        sequenceCounter,
                        traceStage,
                        "compare",
                        isMatch ? "Linear comparison matched row." : "Linear comparison inspected row.",
                        algorithmName,
                        columnName,
                        currentValue,
                        static_cast<int>(i),
                        isMatch
                    );
                }
                if (isMatch) {
                    appendRow(i);
                }
            }
            break;
        }
        case SearchAlgorithm::BINARY: {
            vector<size_t> orderedIndexes(records.size());
            for (size_t i = 0; i < records.size(); ++i) {
                orderedIndexes[i] = i;
            }

            sort(orderedIndexes.begin(), orderedIndexes.end(), [&](size_t lhs, size_t rhs) {
                return isIntColumn
                    ? toIntValue(records[lhs][columnIndex]) < toIntValue(records[rhs][columnIndex])
                    : records[lhs][columnIndex] < records[rhs][columnIndex];
            });

            if (captureObservability) {
                AlgorithmAccess& access = result.algorithmAccess[algorithmName];
                access.detail = "Binary search sorts candidate rows before probing.";
                for (size_t index : orderedIndexes) {
                    access.sortedOrder.push_back(static_cast<int>(index));
                }
                pushTrace(
                    result.trace,
                    sequenceCounter,
                    traceStage,
                    "index_prepare",
                    "Binary search prepared a sorted access order.",
                    algorithmName,
                    columnName,
                    normalizedValue,
                    -1,
                    false,
                    {{"sorted_order_count", to_string(orderedIndexes.size())}}
                );
            }

            long long left = 0;
            long long right = static_cast<long long>(orderedIndexes.size()) - 1;
            long long foundIndex = -1;

            while (left <= right) {
                long long mid = left + (right - left) / 2;
                const string& currentValue = records[orderedIndexes[mid]][columnIndex];
                result.metrics.comparisons++;
                if (isIntColumn) {
                    long long currentInt = toIntValue(currentValue);
                    if (currentInt == targetInt) {
                        foundIndex = mid;
                        if (captureObservability) {
                            recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(orderedIndexes[mid]), result.metrics.comparisons, true, currentValue, "binary-probe");
                            pushTrace(
                                result.trace,
                                sequenceCounter,
                                traceStage,
                                "compare",
                                "Binary search found a matching pivot row.",
                                algorithmName,
                                columnName,
                                currentValue,
                                static_cast<int>(orderedIndexes[mid]),
                                true
                            );
                        }
                        break;
                    }
                    if (captureObservability) {
                        recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(orderedIndexes[mid]), result.metrics.comparisons, false, currentValue, "binary-probe");
                        pushTrace(
                            result.trace,
                            sequenceCounter,
                            traceStage,
                            "compare",
                            "Binary search inspected a pivot row.",
                            algorithmName,
                            columnName,
                            currentValue,
                            static_cast<int>(orderedIndexes[mid]),
                            false
                        );
                    }
                    if (currentInt < targetInt) {
                        left = mid + 1;
                    } else {
                        right = mid - 1;
                    }
                } else {
                    if (currentValue == normalizedValue) {
                        foundIndex = mid;
                        if (captureObservability) {
                            recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(orderedIndexes[mid]), result.metrics.comparisons, true, currentValue, "binary-probe");
                            pushTrace(
                                result.trace,
                                sequenceCounter,
                                traceStage,
                                "compare",
                                "Binary search found a matching pivot row.",
                                algorithmName,
                                columnName,
                                currentValue,
                                static_cast<int>(orderedIndexes[mid]),
                                true
                            );
                        }
                        break;
                    }
                    if (captureObservability) {
                        recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(orderedIndexes[mid]), result.metrics.comparisons, false, currentValue, "binary-probe");
                        pushTrace(
                            result.trace,
                            sequenceCounter,
                            traceStage,
                            "compare",
                            "Binary search inspected a pivot row.",
                            algorithmName,
                            columnName,
                            currentValue,
                            static_cast<int>(orderedIndexes[mid]),
                            false
                        );
                    }
                    if (currentValue < normalizedValue) {
                        left = mid + 1;
                    } else {
                        right = mid - 1;
                    }
                }
            }

            if (foundIndex != -1) {
                long long scan = foundIndex;
                while (scan >= 0) {
                    const string& currentValue = records[orderedIndexes[scan]][columnIndex];
                    result.metrics.comparisons++;
                    bool isMatch = isIntColumn
                        ? toIntValue(currentValue) == targetInt
                        : currentValue == normalizedValue;
                    if (!isMatch) {
                        break;
                    }
                    if (captureObservability) {
                        recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(orderedIndexes[scan]), result.metrics.comparisons, true, currentValue, "binary-scan-left");
                        pushTrace(
                            result.trace,
                            sequenceCounter,
                            traceStage,
                            "match",
                            "Binary search expanded to a matching row on the left.",
                            algorithmName,
                            columnName,
                            currentValue,
                            static_cast<int>(orderedIndexes[scan]),
                            true
                        );
                    }
                    appendRow(orderedIndexes[scan]);
                    scan--;
                }

                scan = foundIndex + 1;
                while (scan < static_cast<long long>(orderedIndexes.size())) {
                    const string& currentValue = records[orderedIndexes[scan]][columnIndex];
                    result.metrics.comparisons++;
                    bool isMatch = isIntColumn
                        ? toIntValue(currentValue) == targetInt
                        : currentValue == normalizedValue;
                    if (!isMatch) {
                        break;
                    }
                    if (captureObservability) {
                        recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(orderedIndexes[scan]), result.metrics.comparisons, true, currentValue, "binary-scan-right");
                        pushTrace(
                            result.trace,
                            sequenceCounter,
                            traceStage,
                            "match",
                            "Binary search expanded to a matching row on the right.",
                            algorithmName,
                            columnName,
                            currentValue,
                            static_cast<int>(orderedIndexes[scan]),
                            true
                        );
                    }
                    appendRow(orderedIndexes[scan]);
                    scan++;
                }
            }
            break;
        }
        case SearchAlgorithm::BTREE: {
            if (isIntColumn) {
                map<long long, vector<size_t>> index;
                for (size_t i = 0; i < records.size(); ++i) {
                    index[toIntValue(records[i][columnIndex])].push_back(i);
                    if (captureObservability) {
                        recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(i), static_cast<int>(i + 1), false, records[i][columnIndex], "btree-index-build");
                    }
                }
                if (captureObservability) {
                    result.algorithmAccess[algorithmName].detail = "BTree search builds an ordered map and probes the target key.";
                    pushTrace(
                        result.trace,
                        sequenceCounter,
                        traceStage,
                        "index_prepare",
                        "BTree-style ordered index constructed for the search column.",
                        algorithmName,
                        columnName,
                        normalizedValue,
                        -1,
                        false
                    );
                }
                result.metrics.comparisons++;
                auto it = index.find(targetInt);
                if (it != index.end()) {
                    result.metrics.comparisons += it->second.size();
                    for (size_t recordIndex : it->second) {
                        if (captureObservability) {
                            recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(recordIndex), result.metrics.comparisons, true, records[recordIndex][columnIndex], "btree-match");
                            pushTrace(
                                result.trace,
                                sequenceCounter,
                                traceStage,
                                "match",
                                "BTree lookup returned a matching row.",
                                algorithmName,
                                columnName,
                                records[recordIndex][columnIndex],
                                static_cast<int>(recordIndex),
                                true
                            );
                        }
                        appendRow(recordIndex);
                    }
                }
            } else {
                map<string, vector<size_t>> index;
                for (size_t i = 0; i < records.size(); ++i) {
                    index[records[i][columnIndex]].push_back(i);
                    if (captureObservability) {
                        recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(i), static_cast<int>(i + 1), false, records[i][columnIndex], "btree-index-build");
                    }
                }
                if (captureObservability) {
                    result.algorithmAccess[algorithmName].detail = "BTree search builds an ordered map and probes the target key.";
                    pushTrace(
                        result.trace,
                        sequenceCounter,
                        traceStage,
                        "index_prepare",
                        "BTree-style ordered index constructed for the search column.",
                        algorithmName,
                        columnName,
                        normalizedValue,
                        -1,
                        false
                    );
                }
                result.metrics.comparisons++;
                auto it = index.find(normalizedValue);
                if (it != index.end()) {
                    result.metrics.comparisons += it->second.size();
                    for (size_t recordIndex : it->second) {
                        if (captureObservability) {
                            recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(recordIndex), result.metrics.comparisons, true, records[recordIndex][columnIndex], "btree-match");
                            pushTrace(
                                result.trace,
                                sequenceCounter,
                                traceStage,
                                "match",
                                "BTree lookup returned a matching row.",
                                algorithmName,
                                columnName,
                                records[recordIndex][columnIndex],
                                static_cast<int>(recordIndex),
                                true
                            );
                        }
                        appendRow(recordIndex);
                    }
                }
            }
            break;
        }
        case SearchAlgorithm::HASH: {
            unordered_map<string, vector<size_t>> index;
            for (size_t i = 0; i < records.size(); ++i) {
                index[records[i][columnIndex]].push_back(i);
                if (captureObservability) {
                    recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(i), static_cast<int>(i + 1), false, records[i][columnIndex], "hash-index-build");
                }
            }
            if (captureObservability) {
                result.algorithmAccess[algorithmName].detail = "Hash search builds a hash table and probes the lookup bucket.";
                pushTrace(
                    result.trace,
                    sequenceCounter,
                    traceStage,
                    "index_prepare",
                    "Hash index constructed for the search column.",
                    algorithmName,
                    columnName,
                    normalizedValue,
                    -1,
                    false
                );
            }
            result.metrics.comparisons++;
            auto it = index.find(isIntColumn ? to_string(targetInt) : normalizedValue);
            if (it != index.end()) {
                result.metrics.comparisons += it->second.size();
                for (size_t recordIndex : it->second) {
                    if (captureObservability) {
                        recordTouch(result.algorithmAccess[algorithmName], static_cast<int>(recordIndex), result.metrics.comparisons, true, records[recordIndex][columnIndex], "hash-match");
                        pushTrace(
                            result.trace,
                            sequenceCounter,
                            traceStage,
                            "match",
                            "Hash lookup returned a matching row.",
                            algorithmName,
                            columnName,
                            records[recordIndex][columnIndex],
                            static_cast<int>(recordIndex),
                            true
                        );
                    }
                    appendRow(recordIndex);
                }
            }
            break;
        }
    }

    if (captureObservability) {
        pushTrace(
            result.trace,
            sequenceCounter,
            traceStage,
            "search_complete",
            "Search execution completed.",
            algorithmName,
            columnName,
            normalizedValue,
            -1,
            !result.rows.empty(),
            {{"matches", to_string(result.rows.size())}, {"comparisons", to_string(result.metrics.comparisons)}}
        );
    }

    return result;
}

TableSearchResult Table::selectRecords(const string& columnName, const string& value, SearchAlgorithm algorithm, bool benchmark) const {
    if (columnName.empty()) {
        TableSearchResult result;
        result.columns = getColumnNames();
        result.rows = records;
        result.metrics.executionTimeMs = 0.0;
        result.metrics.comparisons = records.size();
        return result;
    }

    long long sequenceCounter = 1;

    auto runMeasured = [&](SearchAlgorithm currentAlgorithm, const string& traceStage, size_t iterations = 1, bool captureObservability = true) {
        TableSearchResult measured;
        long long totalMicroseconds = 0;
        const string currentAlgorithmLabel = algorithmLabel(currentAlgorithm);

        for (size_t iteration = 0; iteration < iterations; ++iteration) {
            auto startedAt = chrono::steady_clock::now();
            TableSearchResult current = executeSearch(
                columnName,
                value,
                currentAlgorithm,
                currentAlgorithmLabel,
                traceStage,
                &sequenceCounter,
                captureObservability && iteration == 0
            );
            auto finishedAt = chrono::steady_clock::now();

            if (!current.success) {
                return current;
            }

            if (iteration == 0) {
                measured = current;
            }

            totalMicroseconds += chrono::duration_cast<chrono::microseconds>(finishedAt - startedAt).count();
        }

        measured.metrics.executionTimeMs =
            static_cast<double>(totalMicroseconds) / static_cast<double>(iterations) / 1000.0;
        return measured;
    };

    TableSearchResult result = runMeasured(algorithm, "search");
    if (!result.success) {
        return result;
    }

    if (benchmark) {
        const vector<pair<string, SearchAlgorithm>> algorithms = {
            {"Linear", SearchAlgorithm::LINEAR},
            {"Binary", SearchAlgorithm::BINARY},
            {"BTree", SearchAlgorithm::BTREE},
            {"Hash", SearchAlgorithm::HASH}
        };
        const size_t benchmarkIterations = max<size_t>(10, min<size_t>(records.size(), 200));
        TableSearchResult selectedBenchmarkResult;
        bool hasSelectedBenchmarkResult = false;

        for (const auto& entry : algorithms) {
            TableSearchResult benchmarkResult = runMeasured(entry.second, "benchmark", benchmarkIterations);
            result.benchmark[entry.first] = benchmarkResult.metrics.executionTimeMs;
            result.algorithmAccess[entry.first] = benchmarkResult.algorithmAccess[entry.first];
            result.trace.insert(result.trace.end(), benchmarkResult.trace.begin(), benchmarkResult.trace.end());

            if (entry.second == algorithm) {
                selectedBenchmarkResult = benchmarkResult;
                hasSelectedBenchmarkResult = true;
            }
        }

        if (hasSelectedBenchmarkResult) {
            result.metrics.executionTimeMs = selectedBenchmarkResult.metrics.executionTimeMs;
            result.metrics.comparisons = selectedBenchmarkResult.metrics.comparisons;
            result.algorithmAccess[algorithmLabel(algorithm)] = selectedBenchmarkResult.algorithmAccess[algorithmLabel(algorithm)];
        }
    }

    return result;
}

vector<vector<string>> Table::getRecords() const {
    return records;
}

void Table::setRecords(const vector<vector<string>>& newRecords) {
    records = newRecords;
}

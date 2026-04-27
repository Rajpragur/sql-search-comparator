#pragma once
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <set>

using namespace std;

enum class TxnState { ACTIVE, COMMITTED, ABORTED };

class Transaction {
public:
    int txnId;
    TxnState state;
    
    // Snapshots: table_name -> records. Used for snapshot backing for Abort
    map<string, vector<vector<string>>> tableSnapshots;
    
    // Track tables locked exclusively by this transaction (to prevent duplicate snapshots)
    set<string> writeLockedTables;

    Transaction(int id);
};

class TransactionManager {
private:
    int nextTxnId = 1;
    map<int, Transaction*> activeTxns;
    mutex tmMutex;

public:
    ~TransactionManager();
    
    int beginTransaction();
    bool commitTransaction(int txnId);
    bool abortTransaction(int txnId);
    
    Transaction* getTransaction(int txnId);
};

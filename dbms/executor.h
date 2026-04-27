#pragma once
#include "database.h"
#include "query.h"
#include "transaction.h"
#include "lock_manager.h"

using namespace std;

class Executor {
private:
    Database* db;
    TransactionManager* tm;
    LockManager* lm;
    int currentTxnId;

public:
    Executor(Database* db, TransactionManager* tm, LockManager* lm);
    QueryExecutionResult execute(const Query& query);
};

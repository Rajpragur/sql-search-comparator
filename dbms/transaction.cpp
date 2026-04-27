#include "transaction.h"
#include <iostream>

Transaction::Transaction(int id) : txnId(id), state(TxnState::ACTIVE) {}

TransactionManager::~TransactionManager() {
    for (auto const& [id, txn] : activeTxns) {
        delete txn;
    }
}

int TransactionManager::beginTransaction() {
    lock_guard<mutex> lock(tmMutex);
    int id = nextTxnId++;
    activeTxns[id] = new Transaction(id);
    return id;
}

bool TransactionManager::commitTransaction(int txnId) {
    lock_guard<mutex> lock(tmMutex);
    auto it = activeTxns.find(txnId);
    if (it != activeTxns.end() && it->second->state == TxnState::ACTIVE) {
        it->second->state = TxnState::COMMITTED;
        return true;
    }
    return false;
}

bool TransactionManager::abortTransaction(int txnId) {
    lock_guard<mutex> lock(tmMutex);
    auto it = activeTxns.find(txnId);
    if (it != activeTxns.end() && it->second->state == TxnState::ACTIVE) {
        it->second->state = TxnState::ABORTED;
        return true;
    }
    return false;
}

Transaction* TransactionManager::getTransaction(int txnId) {
    lock_guard<mutex> lock(tmMutex);
    auto it = activeTxns.find(txnId);
    if (it != activeTxns.end()) {
        return it->second;
    }
    return nullptr;
}

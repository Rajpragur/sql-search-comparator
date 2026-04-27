#include "lock_manager.h"

#include <algorithm>
#include <functional>
#include <iostream>

namespace {
bool containsWaiter(const vector<pair<int, LockType>>& waitingRequests, int txnId, LockType type) {
  return any_of(waitingRequests.begin(), waitingRequests.end(), [&](const auto& request) {
    return request.first == txnId && request.second == type;
  });
}

void eraseWaiter(vector<pair<int, LockType>>& waitingRequests, int txnId, LockType type) {
  waitingRequests.erase(
      remove_if(waitingRequests.begin(), waitingRequests.end(), [&](const auto& request) {
        return request.first == txnId && request.second == type;
      }),
      waitingRequests.end());
}

bool canGrantShared(const LockInfo& info, int txnId) {
  return info.exclusiveHolder == -1 || info.exclusiveHolder == txnId;
}

bool canGrantExclusive(const LockInfo& info, int txnId) {
  return info.exclusiveHolder == -1 &&
         (info.sharedHolders.empty() ||
          (info.sharedHolders.size() == 1 && info.sharedHolders.count(txnId) == 1));
}
}

string LockManager::lockTypeName(LockType type) {
  return type == LockType::SHARED ? "SHARED" : "EXCLUSIVE";
}

void LockManager::emitTraceLocked(
    const string& stage,
    const string& type,
    const string& message,
    int txnId,
    const string& tableName,
    LockType lockType,
    const string& status) {
  if (!activeTrace || !sequenceCounter) {
    return;
  }

  TraceEvent event;
  event.id = (*sequenceCounter)++;
  event.stage = stage;
  event.type = type;
  event.message = message;
  event.txnId = txnId;
  event.tableName = tableName;
  event.lockType = lockTypeName(lockType);
  event.status = status;
  activeTrace->push_back(event);
}

vector<WaitEdge> LockManager::buildWaitForGraphLocked() const {
  vector<WaitEdge> edges;

  for (const auto& [tableName, info] : lockTable) {
    for (const auto& [waitingTxnId, requestedType] : info.waitingRequests) {
      if (info.exclusiveHolder != -1 && info.exclusiveHolder != waitingTxnId) {
        edges.push_back({waitingTxnId, info.exclusiveHolder, tableName, lockTypeName(requestedType)});
      }

      for (int sharedHolder : info.sharedHolders) {
        if (sharedHolder == waitingTxnId) {
          continue;
        }
        if (requestedType == LockType::EXCLUSIVE || info.exclusiveHolder != -1) {
          edges.push_back({waitingTxnId, sharedHolder, tableName, lockTypeName(requestedType)});
        }
      }
    }
  }

  return edges;
}

bool LockManager::hasDeadlockCycleLocked() const {
  map<int, vector<int>> adjacency;
  for (const auto& edge : buildWaitForGraphLocked()) {
    adjacency[edge.fromTxnId].push_back(edge.toTxnId);
  }

  set<int> visiting;
  set<int> visited;
  function<bool(int)> dfs = [&](int node) {
    if (visiting.count(node)) {
      return true;
    }
    if (visited.count(node)) {
      return false;
    }

    visiting.insert(node);
    for (int neighbor : adjacency[node]) {
      if (dfs(neighbor)) {
        return true;
      }
    }
    visiting.erase(node);
    visited.insert(node);
    return false;
  };

  for (const auto& [node, _] : adjacency) {
    if (dfs(node)) {
      return true;
    }
  }

  return false;
}

bool LockManager::acquireLock(int txnId, const string& tableName, LockType type) {
  unique_lock<mutex> lock(mtx);
  LockInfo& info = lockTable[tableName];

  emitTraceLocked("lock", "request", "Lock requested.", txnId, tableName, type, "requested");

  if (type == LockType::SHARED) {
    while (!canGrantShared(info, txnId)) {
      if (!containsWaiter(info.waitingRequests, txnId, type)) {
        info.waitingRequests.push_back({txnId, type});
        emitTraceLocked("lock", "wait", "Waiting for shared lock.", txnId, tableName, type, "waiting");
      }
      cv.wait(lock);
    }

    eraseWaiter(info.waitingRequests, txnId, type);
    info.sharedHolders.insert(txnId);
    txnLocks[txnId].insert(tableName);
    cout << "[LockManager] Txn " << txnId << " acquired SHARED lock on '"
         << tableName << "'\n";
    emitTraceLocked("lock", "acquire", "Shared lock acquired.", txnId, tableName, type, "granted");
  } else {
    cout << "[LockManager] Txn " << txnId << " requesting EXCLUSIVE lock on '"
         << tableName << "'...\n";

    while (!canGrantExclusive(info, txnId)) {
      if (!containsWaiter(info.waitingRequests, txnId, type)) {
        info.waitingRequests.push_back({txnId, type});
        emitTraceLocked("lock", "wait", "Waiting for exclusive lock.", txnId, tableName, type, "waiting");
      }
      cv.wait(lock);
    }

    eraseWaiter(info.waitingRequests, txnId, type);
    info.exclusiveHolder = txnId;
    txnLocks[txnId].insert(tableName);
    cout << "[LockManager] Txn " << txnId << " acquired EXCLUSIVE lock on '"
         << tableName << "'\n";
    emitTraceLocked("lock", "acquire", "Exclusive lock acquired.", txnId, tableName, type, "granted");
  }
  return true;
}

void LockManager::releaseAllLocks(int txnId) {
  lock_guard<mutex> lock(mtx);
  auto it = txnLocks.find(txnId);
  if (it != txnLocks.end()) {
    for (const string& table : it->second) {
      LockInfo& info = lockTable[table];
      info.sharedHolders.erase(txnId);
      if (info.exclusiveHolder == txnId) {
        info.exclusiveHolder = -1;
      }
      emitTraceLocked("cleanup", "release", "Released lock on table.", txnId, table, LockType::EXCLUSIVE, "released");
    }
    txnLocks.erase(it);
    cout << "[LockManager] Txn " << txnId << " released all locks.\n";
    cv.notify_all();
  }
}

void LockManager::setTraceBuffer(vector<TraceEvent>* traceBuffer, long long* nextSequence) {
  lock_guard<mutex> lock(mtx);
  activeTrace = traceBuffer;
  sequenceCounter = nextSequence;
}

void LockManager::clearTraceBuffer() {
  lock_guard<mutex> lock(mtx);
  activeTrace = nullptr;
  sequenceCounter = nullptr;
}

vector<LockSnapshot> LockManager::getLockSnapshots() const {
  lock_guard<mutex> lock(mtx);
  vector<LockSnapshot> snapshots;

  for (const auto& [tableName, info] : lockTable) {
    LockSnapshot snapshot;
    snapshot.tableName = tableName;
    snapshot.exclusiveHolder = info.exclusiveHolder;
    snapshot.sharedHolders.assign(info.sharedHolders.begin(), info.sharedHolders.end());

    for (const auto& [txnId, type] : info.waitingRequests) {
      if (type == LockType::SHARED) {
        snapshot.waitingShared.push_back(txnId);
      } else {
        snapshot.waitingExclusive.push_back(txnId);
      }
    }

    if (snapshot.exclusiveHolder != -1 || !snapshot.sharedHolders.empty() ||
        !snapshot.waitingShared.empty() || !snapshot.waitingExclusive.empty()) {
      snapshots.push_back(snapshot);
    }
  }

  return snapshots;
}

vector<WaitEdge> LockManager::getWaitForGraph() const {
  lock_guard<mutex> lock(mtx);
  return buildWaitForGraphLocked();
}

bool LockManager::hasDeadlockRisk() const {
  lock_guard<mutex> lock(mtx);
  return hasDeadlockCycleLocked();
}

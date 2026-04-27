#pragma once
#include <string>
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "query.h"

using namespace std;

enum class LockType { SHARED, EXCLUSIVE };

struct LockInfo {
    int exclusiveHolder = -1; // -1 if no exclusive lock
    set<int> sharedHolders;
    vector<pair<int, LockType>> waitingRequests;
};

class LockManager {
private:
    mutable mutex mtx;
    condition_variable cv;
    map<string, LockInfo> lockTable;
    // Map txnId to tables locked by it for easy release
    map<int, set<string>> txnLocks;
    vector<TraceEvent>* activeTrace = nullptr;
    long long* sequenceCounter = nullptr;

    void emitTraceLocked(
        const string& stage,
        const string& type,
        const string& message,
        int txnId,
        const string& tableName,
        LockType lockType,
        const string& status
    );
    vector<WaitEdge> buildWaitForGraphLocked() const;
    bool hasDeadlockCycleLocked() const;
    static string lockTypeName(LockType type);

public:
    // Blocking call to acquire a lock
    bool acquireLock(int txnId, const string& tableName, LockType type);
    
    // Release all locks held by a transaction (during COMMIT/ABORT)
    void releaseAllLocks(int txnId);
    void setTraceBuffer(vector<TraceEvent>* traceBuffer, long long* nextSequence);
    void clearTraceBuffer();
    vector<LockSnapshot> getLockSnapshots() const;
    vector<WaitEdge> getWaitForGraph() const;
    bool hasDeadlockRisk() const;
};

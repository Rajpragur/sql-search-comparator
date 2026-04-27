# Technical Project Documentation: Custom DBMS Analysis

This document provides a deep-dive into the implementation details of the **Design and Comparative Analysis of Search and Concurrency Strategies in a Custom DBMS** project.

## 1. DBMS Core Implementation (C++)

The DBMS is designed with a modular architecture to allow easy comparison of different strategies.

### A. Search Algorithms
Located in `dbms/table.cpp`, the `executeSearch` method implements the following:

- **Linear Search**: Iterates through every record in the table. It is the fallback when no index is available or specified.
- **Binary Search**: Performs a logarithmic search. It tracks the number of "comparisons" to provide benchmarking data.
- **B-Tree**: A custom B-Tree implementation that maintains balanced nodes. It is ideal for both point queries and range scans.
- **Hash Index**: Uses an unordered map-like structure for $O(1)$ lookups. It is the fastest for equality checks but doesn't support range queries.

### B. Concurrency Control (2PL)
Implemented in `dbms/lock_manager.cpp`, the system follows the **Strict Two-Phase Locking** protocol:

1.  **Growing Phase**: A transaction acquires locks as it accesses data.
    -   **Shared (S) Locks**: For `SELECT` operations. Multiple transactions can hold S-locks on the same table.
    -   **Exclusive (X) Locks**: For `INSERT`, `UPDATE`, or `DELETE`. Only one transaction can hold an X-lock, and it blocks all other S and X requests.
2.  **Shrinking Phase**: All locks held by a transaction are released simultaneously when the transaction either **Commits** or **Aborts**.

### C. Deadlock Management
The `LockManager` maintains a dependency graph (Wait-For Graph).
-   If Transaction A holds a lock that Transaction B needs, an edge $B \rightarrow A$ is added.
-   The system periodically runs a **Cycle Detection** algorithm (DFS).
-   If a cycle is detected, the `deadlockRisk` flag is set, which is then visualized in the frontend.

---

## 2. Backend Orchestration (Python/Flask)

The backend (`backend/app.py`) solves the challenge of providing a persistent state over a stateless C++ CLI.

### State Persistence Trick
Every time a query is executed, the backend:
1.  Identifies all "Persistable" statements (`CREATE`, `INSERT`) in the session history.
2.  Prepends these statements to the current query.
3.  Executes the entire sequence in the C++ binary.
This ensures that the "in-memory" database in C++ appears persistent to the user.

### Observability Data
The C++ DBMS outputs a rich JSON structure containing:
-   `trace`: A chronological list of internal steps (e.g., "Compared key 50 with 75").
-   `metrics`: Execution time and comparison counts.
-   `lock_snapshots`: The current state of the lock table.

---

## 3. Frontend Visualization (React)

The frontend uses this observability data to create an immersive experience.

### Key Components:
-   **ExecutionTimeline.jsx**: Renders the `trace` data as a vertical timeline, showing exactly how the algorithm "thought" through the search.
-   **ComparisonHeatmap.jsx**: Compares different algorithms side-by-side using the `benchmark` data.
-   **LockGraphPanel.jsx**: Uses a graph visualization to show transactions as nodes and lock dependencies as edges.

---

## 4. Query Language Support

The custom parser (`dbms/parser.cpp`) supports a subset of SQL:

| Command | Description |
| :--- | :--- |
| `CREATE TABLE name (col1, col2);` | Defines a new schema. |
| `INSERT INTO name VALUES (val1, val2);` | Adds data. |
| `SELECT * FROM name WHERE col = val;` | Search with default algorithm. |
| `SELECT ... USING [BTREE\|HASH\|BINARY];` | Explicitly choose a search strategy. |
| `BEGIN;`, `COMMIT;`, `ABORT;` | Transaction boundaries. |

---

## 5. Performance Metrics

The project captures two primary metrics for comparison:
1.  **Wall-clock Time (ms)**: Actual execution duration.
2.  **Comparison Count**: The number of key-to-key comparisons made. This is a more "stable" metric for algorithm analysis than time, which can fluctuate based on CPU load.
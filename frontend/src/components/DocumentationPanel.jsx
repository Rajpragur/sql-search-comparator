const sections = [
    {
        title: 'How The Simulator Works',
        description: 'The simulator sends your SQL-like command to the Python backend, which forwards it to the C++ DBMS engine. The engine parses the query, acquires locks, runs the table operation, measures search cost, and returns rows, logs, comparisons, and benchmark timings to the UI.',
        example: `SELECT * FROM books WHERE accession_no = 123;`
    },
    {
        title: 'Create Tables',
        description: 'Create a table with named columns and data types. Integer columns are validated as numbers. String columns accept quoted text values.',
        example: `CREATE TABLE students (id INT, name STRING, age INT);`
    },
    {
        title: 'Insert Records',
        description: 'Insert one row at a time in the same order as the table columns. The number of values must match the number of columns exactly.',
        example: `INSERT INTO students VALUES (1, 'Aarav', 13);\nINSERT INTO students VALUES (2, 'Meera', 15);\nINSERT INTO students VALUES (3, 'Kabir', 13);`
    },
    {
        title: 'Select And Search',
        description: 'Run a selection query to search rows by a target column. The dropdown chooses the active search algorithm used for the summary execution time and comparison count.',
        example: `SELECT * FROM students WHERE age = 13;\nSELECT * FROM students WHERE name = 'Meera';`
    },
    {
        title: 'Search Algorithms',
        description: 'Linear scans every row in sequence. Binary sorts by the target column and then performs binary search. BTree builds an ordered map-style index. Hash builds a hash map for direct lookup on the searched value.',
        example: `SELECT * FROM students WHERE age = 13 USING LINEAR;\nSELECT * FROM students WHERE age = 13 USING BINARY;\nSELECT * FROM students WHERE age = 13 USING BTREE;\nSELECT * FROM students WHERE age = 13 USING HASH;`
    },
    {
        title: 'Benchmarking',
        description: 'When Benchmark is enabled, the simulator measures all four search algorithms for the same query and plots their execution times in the graph. This lets you compare which method is fastest for the current dataset and condition.',
        example: `CREATE TABLE benchmark_demo (id INT, name STRING, age INT);\nINSERT INTO benchmark_demo VALUES (1, 'Ria', 20);\nINSERT INTO benchmark_demo VALUES (2, 'Dev', 21);\nINSERT INTO benchmark_demo VALUES (3, 'Neel', 20);\nSELECT * FROM benchmark_demo WHERE age = 20;`
    },
    {
        title: 'Execution Time And Comparisons',
        description: 'Execution Time is the measured time taken by the selected search function to execute for the query. Comparisons is the number of value checks that algorithm performed while searching for the matching rows.',
        example: `SELECT * FROM students WHERE age = 13;`
    },
    {
        title: 'Transactions',
        description: 'Use BEGIN to start a transaction, COMMIT to keep the changes, and ABORT to roll them back. This helps demonstrate controlled multi-step updates with recovery support.',
        example: `BEGIN;\nINSERT INTO students VALUES (4, 'Ishaan', 16);\nINSERT INTO students VALUES (5, 'Sara', 14);\nCOMMIT;`
    },
    {
        title: 'Abort And Rollback',
        description: 'If a transaction is aborted, the engine restores the table to its snapshot from before the transaction began. This shows how rollback preserves consistency when work should not be saved.',
        example: `BEGIN;\nINSERT INTO students VALUES (6, 'Temp', 99);\nABORT;\nSELECT * FROM students;`
    },
    {
        title: 'Lock Manager And Concurrency',
        description: 'The Lock Manager protects shared data during query execution. SELECT acquires a shared lock, while CREATE, INSERT, and DROP acquire exclusive locks. This prevents conflicting access and supports two-phase locking behavior in the simulator logs.',
        example: `CREATE TABLE accounts (id INT, owner STRING, balance INT);\nBEGIN;\nINSERT INTO accounts VALUES (1, 'Asha', 500);\nCOMMIT;`
    },
    {
        title: 'Table Explorer And Deletion',
        description: 'The explorer on the right shows known tables, lets you inspect their contents, and allows deleting tables other than the default books table. Deletion updates the simulator state and clears related history.',
        example: `DROP TABLE benchmark_demo;`
    }
];

export default function DocumentationPanel() {
    return (
        <section className="panel documentation-panel">
            <div className="panel-header">
                <div>
                    <div className="panel-kicker">Simulator Guide</div>
                    <h2 className="panel-title panel-title--small">Documentation</h2>
                </div>
                <span className="status-chip">In-App Manual</span>
            </div>

            <div className="documentation-intro">
                This console demonstrates a small DBMS workflow with parsing, transactions, locking, searchable records, and benchmarked query execution. Use the examples below directly in the editor to validate each supported feature.
            </div>

            <div className="documentation-grid">
                {sections.map((section) => (
                    <article key={section.title} className="doc-card">
                        <h3 className="doc-card-title">{section.title}</h3>
                        <p className="doc-card-text">{section.description}</p>
                        <pre className="doc-card-example">{section.example}</pre>
                    </article>
                ))}
            </div>
        </section>
    );
}

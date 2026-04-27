const syntaxSections = [
    {
        id: 'overview',
        title: 'Simulator Overview',
        body: 'This simulator demonstrates a compact DBMS stack. The React UI sends SQL-like commands to the Flask backend, the backend invokes the C++ DBMS executable, and the DBMS parses, locks, executes, benchmarks, traces, and returns structured results. It is designed for learning database internals rather than replacing MySQL or PostgreSQL.',
        syntax: `-- Start with the built-in books table\nSELECT * FROM books;\nSELECT * FROM books WHERE accession_no = 123;`
    },
    {
        id: 'create',
        title: 'CREATE TABLE',
        body: 'Creates a new in-memory table. The simulator supports column names with two data types: INT and STRING. INT values are validated during insertion and search. STRING values can be inserted using single or double quotes.',
        syntax: `CREATE TABLE table_name (column_name INT, column_name STRING);\n\nCREATE TABLE students (id INT, name STRING, age INT);\nCREATE TABLE accounts (id INT, owner STRING, balance INT);`
    },
    {
        id: 'insert',
        title: 'INSERT INTO',
        body: 'Adds one record to an existing table. Values must be provided in the exact same order as the table definition. The current parser does not support column-specific INSERT syntax such as INSERT INTO table (a, b).',
        syntax: `INSERT INTO table_name VALUES (value1, 'value2', value3);\n\nINSERT INTO students VALUES (1, 'Aarav', 13);\nINSERT INTO students VALUES (2, 'Meera', 15);\nINSERT INTO accounts VALUES (1, 'Shiva', 500);`
    },
    {
        id: 'select-all',
        title: 'SELECT All Rows',
        body: 'Fetches all rows from a table. This path does not use a WHERE search algorithm because no target value is being searched; it simply returns the table contents.',
        syntax: `SELECT * FROM table_name;\n\nSELECT * FROM books;\nSELECT * FROM students;`
    },
    {
        id: 'select-where',
        title: 'SELECT With WHERE',
        body: 'Searches rows by comparing a column against a target value. The simulator supports equality conditions using the = operator. Use quotes for string targets and raw numbers for integer targets.',
        syntax: `SELECT * FROM table_name WHERE column_name = value;\nSELECT * FROM table_name WHERE column_name = 'text';\n\nSELECT * FROM students WHERE age = 13;\nSELECT * FROM students WHERE name = 'Meera';`
    },
    {
        id: 'algorithms',
        title: 'Search Algorithms',
        body: 'The DBMS supports LINEAR, BINARY, BTREE, and HASH search. In the frontend, the dropdown is authoritative for SELECT queries: the UI rewrites SELECT statements to use the selected dropdown algorithm. You can still use USING syntax directly when running the DBMS executable.',
        syntax: `SELECT * FROM students WHERE age = 13 USING LINEAR;\nSELECT * FROM students WHERE age = 13 USING BINARY;\nSELECT * FROM students WHERE age = 13 USING BTREE;\nSELECT * FROM students WHERE age = 13 USING HASH;`
    },
    {
        id: 'linear',
        title: 'LINEAR Search',
        body: 'Linear search checks rows one by one. It is easy to understand and useful for demonstration, but comparisons usually grow with the number of records.',
        syntax: `-- Dropdown: Linear\nSELECT * FROM students WHERE age = 13;`
    },
    {
        id: 'binary',
        title: 'BINARY Search',
        body: 'Binary search creates a sorted order for the searched column and probes the middle of that order. The comparison heatmap can show the sorted access path versus original row indices.',
        syntax: `-- Dropdown: Binary\nSELECT * FROM students WHERE age = 13;`
    },
    {
        id: 'btree',
        title: 'BTREE Search',
        body: 'The simulator models BTree-style lookup with an ordered map from search key to matching row positions. It demonstrates indexed lookup behavior and key-based matching.',
        syntax: `-- Dropdown: BTree\nSELECT * FROM students WHERE age = 13;`
    },
    {
        id: 'hash',
        title: 'HASH Search',
        body: 'Hash search builds a hash table from the searched column and probes the bucket for the target value. It is useful for equality lookup demonstrations.',
        syntax: `-- Dropdown: Hash\nSELECT * FROM students WHERE name = 'Aarav';`
    },
    {
        id: 'benchmarking',
        title: 'Benchmarking',
        body: 'When Benchmark is enabled, the DBMS measures all four algorithms for the same SELECT condition and returns a timing profile. The graph and Algorithm Timings panel show per-algorithm execution time in milliseconds. The Execution Stats panel reflects the selected dropdown algorithm.',
        syntax: `CREATE TABLE benchmark_demo (id INT, name STRING, age INT);\nINSERT INTO benchmark_demo VALUES (1, 'Ria', 20);\nINSERT INTO benchmark_demo VALUES (2, 'Dev', 21);\nINSERT INTO benchmark_demo VALUES (3, 'Neel', 20);\nSELECT * FROM benchmark_demo WHERE age = 20;`
    },
    {
        id: 'transactions',
        title: 'Transactions',
        body: 'BEGIN starts a transaction. COMMIT permanently keeps changes. ABORT restores snapshots captured before write operations. If no transaction is active, the executor uses auto-commit for individual statements.',
        syntax: `BEGIN;\nINSERT INTO students VALUES (4, 'Ishaan', 16);\nINSERT INTO students VALUES (5, 'Sara', 14);\nCOMMIT;`
    },
    {
        id: 'abort',
        title: 'ABORT And Rollback',
        body: 'ABORT cancels the active transaction and restores table snapshots. This is visible in the command logs and execution timeline.',
        syntax: `BEGIN;\nINSERT INTO students VALUES (99, 'Temporary', 99);\nABORT;\nSELECT * FROM students;`
    },
    {
        id: 'drop',
        title: 'DROP TABLE',
        body: 'Deletes a user-created table from the simulator state. The built-in books table is protected and cannot be dropped.',
        syntax: `DROP TABLE table_name;\n\nDROP TABLE benchmark_demo;`
    },
    {
        id: 'multi',
        title: 'Multi-Statement Input',
        body: 'The DBMS executes multiple commands when they are separated by semicolons. New lines are allowed for readability, but semicolons are the actual statement separators.',
        syntax: `CREATE TABLE courses (id INT, title STRING);\nINSERT INTO courses VALUES (1, 'DBMS');\nINSERT INTO courses VALUES (2, 'Operating Systems');\nSELECT * FROM courses;`
    }
];

const visualizationSections = [
    {
        title: 'Execution Stats',
        detail: 'Shows selected algorithm, execution time in milliseconds, and comparison count. Execution time is the measured search-function runtime. Comparisons are the value checks performed while searching.'
    },
    {
        title: 'Command Response',
        detail: 'Shows success/failure status, backend message, and DBMS logs including lock acquisition, transaction commits, abort rollback messages, and table output.'
    },
    {
        title: 'Matched Records',
        detail: 'Displays records returned by SELECT queries. Empty query results show a compact placeholder instead of a table.'
    },
    {
        title: 'Database Explorer',
        detail: 'Lists known tables, opens table contents, and allows deleting user-created tables. The protected books table remains available as the default dataset.'
    },
    {
        title: 'Execution Graph',
        detail: 'Bar chart of benchmark timings for Linear, Binary, BTree, and Hash. It is stable for the same query/data and helps compare algorithm performance.'
    },
    {
        title: 'Comparison Heatmap',
        detail: 'Shows which rows each algorithm touched, compared, indexed, or matched. Amber means touched/compared, green means matched, and blank means not accessed.'
    },
    {
        title: 'Execution Timeline',
        detail: 'Step-by-step playback of parse, transaction, lock, search, benchmark, mutation, commit, abort, rollback, and cleanup events.'
    },
    {
        title: 'Lock Graph',
        detail: 'Shows lock ownership, waiting edges, and deadlock-risk state for the executed query. With the current subprocess engine, this is a per-query teaching view rather than a persistent multi-client live server.'
    }
];

const limitations = [
    'Only equality WHERE conditions are currently supported.',
    'INSERT requires values in table-column order and does not support explicit column lists.',
    'The frontend dropdown overrides trailing USING clauses for SELECT queries.',
    'Semicolons separate statements; line breaks alone do not execute separate commands.',
    'The database state is reconstructed by the backend during requests and is not a production-grade persistent storage engine.',
    'The live lock graph is currently per-query because the DBMS executable is invoked per request.'
];

export default function DocumentationPage({ onClose }) {
    return (
        <div className="docs-page" role="dialog" aria-modal="true" aria-label="Simulator documentation">
            <div className="docs-page-topbar">
                <div>
                    <div className="panel-kicker">Official Simulator Documentation</div>
                    <h1 className="docs-page-title">DBMS Console Docs</h1>
                </div>
                <button className="docs-close-button" type="button" onClick={onClose}>Back to Simulator</button>
            </div>

            <div className="docs-page-layout">
                <nav className="docs-sidebar" aria-label="Documentation sections">
                    <a href="#quickstart">Quickstart</a>
                    {syntaxSections.map((section) => (
                        <a key={section.id} href={`#${section.id}`}>{section.title}</a>
                    ))}
                    <a href="#visualizations">Visualizations</a>
                    <a href="#limitations">Current Limits</a>
                </nav>

                <main className="docs-content">
                    <section id="quickstart" className="docs-hero panel">
                        <div className="panel-kicker">Quickstart</div>
                        <h2>Run this first</h2>
                        <p>
                            Paste the following block into the editor and press COMPILE. It creates a table, inserts rows, searches with the selected dropdown algorithm, and populates results, benchmark charts, heatmap, timeline, and lock views.
                        </p>
                        <pre className="docs-code">{`CREATE TABLE students (id INT, name STRING, age INT);\nINSERT INTO students VALUES (1, 'Aarav', 13);\nINSERT INTO students VALUES (2, 'Meera', 15);\nINSERT INTO students VALUES (3, 'Kabir', 13);\nSELECT * FROM students WHERE age = 13;`}</pre>
                    </section>

                    <section className="docs-section">
                        <div className="panel-kicker">SQL Reference</div>
                        <h2>Supported SQL Syntax</h2>
                        <p>
                            The simulator intentionally supports a focused SQL-like subset so DBMS internals are easy to inspect. Every supported command and syntax pattern is listed below.
                        </p>
                        <div className="docs-card-grid">
                            {syntaxSections.map((section) => (
                                <article id={section.id} key={section.id} className="docs-card">
                                    <h3>{section.title}</h3>
                                    <p>{section.body}</p>
                                    <pre className="docs-code">{section.syntax}</pre>
                                </article>
                            ))}
                        </div>
                    </section>

                    <section id="visualizations" className="docs-section">
                        <div className="panel-kicker">Visualization Reference</div>
                        <h2>How to read every panel</h2>
                        <div className="docs-feature-grid">
                            {visualizationSections.map((section) => (
                                <article key={section.title} className="docs-feature-card">
                                    <h3>{section.title}</h3>
                                    <p>{section.detail}</p>
                                </article>
                            ))}
                        </div>
                    </section>

                    <section id="limitations" className="docs-section panel docs-limitations">
                        <div className="panel-kicker">Current Behavior</div>
                        <h2>Important implementation notes</h2>
                        <ul>
                            {limitations.map((item) => (
                                <li key={item}>{item}</li>
                            ))}
                        </ul>
                    </section>
                </main>
            </div>
        </div>
    );
}

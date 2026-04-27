import { useEffect, useState } from 'react';
import QueryPanel from './components/QueryPanel.jsx';
import SummaryPanel from './components/SummaryPanel.jsx';
import QueryOutputPanel from './components/QueryOutputPanel.jsx';
import ResultsPanel from './components/ResultsPanel.jsx';
import ExplorerPanel from './components/ExplorerPanel.jsx';
import SelectedTablePanel from './components/SelectedTablePanel.jsx';
import BenchmarkPanel from './components/BenchmarkPanel.jsx';
import ChartPanel from './components/ChartPanel.jsx';
import DocumentationPanel from './components/DocumentationPanel.jsx';
import DocumentationPage from './components/DocumentationPage.jsx';
import ExecutionTimeline from './components/ExecutionTimeline.jsx';
import LockGraphPanel from './components/LockGraphPanel.jsx';
import ComparisonHeatmap from './components/ComparisonHeatmap.jsx';
import { extractTargetTableName, getApiBase, normalizeQuery } from './utils.js';

export default function App() {
    const [queryInput, setQueryInput] = useState('SELECT * FROM books WHERE accession_no = 123;');
    const [algorithm, setAlgorithm] = useState('BINARY');
    const [benchmarkEnabled, setBenchmarkEnabled] = useState(true);
    const [loading, setLoading] = useState(false);
    const [errorMessage, setErrorMessage] = useState('');
    const [warningMessage, setWarningMessage] = useState('');
    const [results, setResults] = useState([]);
    const [metrics, setMetrics] = useState({ execution_time_ms: 0, comparisons: 0, search_algorithm: '' });
    const [benchmark, setBenchmark] = useState({});
    const [queryOutput, setQueryOutput] = useState({
        success: true,
        title: 'Awaiting query execution',
        message: 'Run any SQL command to see the DBMS response here.',
        logs: 'No execution logs yet.'
    });
    const [trace, setTrace] = useState([]);
    const [algorithmAccess, setAlgorithmAccess] = useState({});
    const [lockSnapshots, setLockSnapshots] = useState([]);
    const [waitForGraph, setWaitForGraph] = useState([]);
    const [deadlockRisk, setDeadlockRisk] = useState(false);
    const [tables, setTables] = useState([]);
    const [selectedTable, setSelectedTable] = useState(null);
    const [selectedTableRows, setSelectedTableRows] = useState([]);
    const [selectedTableMessage, setSelectedTableMessage] = useState('Choose any existing table to inspect it in a dedicated view.');
    const [documentationOpen, setDocumentationOpen] = useState(false);

    async function openTable(tableName) {
        setSelectedTable(tableName);
        setSelectedTableMessage(`Loading "${tableName}"...`);
        setSelectedTableRows([]);

        try {
            const response = await fetch(`${getApiBase()}/tables/${encodeURIComponent(tableName)}`);
            const data = await response.json();
            if (!response.ok) {
                throw new Error(data.error || 'Unable to load table.');
            }

            setSelectedTable(tableName);
            setSelectedTableRows(data.results || []);
            setSelectedTableMessage(
                (data.results || []).length
                    ? `Showing the current contents of "${tableName}" in a dedicated table browser.`
                    : `Table "${tableName}" is currently empty.`
            );
        } catch (error) {
            setSelectedTable(tableName);
            setSelectedTableRows([]);
            setSelectedTableMessage(error.message || 'Unable to load table.');
        }
    }

    async function syncExistingTables(nextTables, preferredTable = null) {
        setTables(nextTables);

        const tableToOpen = preferredTable && nextTables.includes(preferredTable)
            ? preferredTable
            : nextTables[0] || null;

        if (tableToOpen) {
            await openTable(tableToOpen);
        } else {
            setSelectedTable(null);
            setSelectedTableRows([]);
            setSelectedTableMessage('Choose any existing table to inspect it in a dedicated view.');
        }
    }

    async function loadExistingTables(preferredTable = null) {
        try {
            const response = await fetch(`${getApiBase()}/tables`);
            const data = await response.json();
            if (!response.ok) {
                throw new Error(data.error || 'Unable to load tables.');
            }

            await syncExistingTables(data.tables || [], preferredTable);
        } catch (error) {
            setTables([]);
            setSelectedTable(null);
            setSelectedTableRows([]);
            setSelectedTableMessage(error.message || 'Unable to load tables.');
        }
    }

    async function deleteTable(tableName) {
        if (!window.confirm(`Delete table "${tableName}" from the database?`)) {
            return;
        }

        setErrorMessage('');

        try {
            const response = await fetch(`${getApiBase()}/tables/${encodeURIComponent(tableName)}`, {
                method: 'DELETE'
            });
            const data = await response.json();
            if (!response.ok) {
                throw new Error(data.error || 'Unable to delete table.');
            }

            setQueryOutput({
                success: data.success !== false,
                title: 'Command executed successfully',
                message: data.message || 'Table deleted successfully.',
                logs: (data.logs || '').trim() || 'No execution log was returned for this command.'
            });
            setResults([]);
            setMetrics({ execution_time_ms: 0, comparisons: 0, search_algorithm: '' });
            setBenchmark({});
            setWarningMessage('');
            setTrace(data.trace || []);
            setAlgorithmAccess(data.algorithm_access || {});
            setLockSnapshots(data.lock_snapshots || []);
            setWaitForGraph(data.wait_for_graph || []);
            setDeadlockRisk(Boolean(data.deadlock_risk));

            const nextTables = data.existing_tables || [];
            const nextPreferred = nextTables.find((name) => name !== tableName) || null;
            await syncExistingTables(nextTables, nextPreferred);
        } catch (error) {
            setErrorMessage(error.message || 'Unable to delete table.');
        }
    }

    async function runQuery() {
        setErrorMessage('');
        setWarningMessage('');

        const userQuery = normalizeQuery(queryInput, algorithm);
        if (!userQuery) {
            setErrorMessage('Please enter a SQL query before running.');
            return;
        }

        setLoading(true);

        try {
            const response = await fetch(`${getApiBase()}/executeQuery`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    query: userQuery,
                    benchmark: benchmarkEnabled
                })
            });

            const data = await response.json();
            if (!response.ok) {
                throw new Error(data.error || 'Query execution failed.');
            }

            const nextResults = data.results || [];
            setResults(nextResults);
            setMetrics({
                execution_time_ms: data.execution_time_ms ?? 0,
                comparisons: data.comparisons ?? 0,
                search_algorithm: data.search_algorithm || ''
            });
            setBenchmark(data.benchmark || {});
            setTrace(data.trace || []);
            setAlgorithmAccess(data.algorithm_access || {});
            setLockSnapshots(data.lock_snapshots || []);
            setWaitForGraph(data.wait_for_graph || []);
            setDeadlockRisk(Boolean(data.deadlock_risk));
            setQueryOutput({
                success: data.success !== false,
                title: nextResults.length ? 'Query returned matching records' : 'Command executed successfully',
                message: data.message || 'Query executed.',
                logs: (data.logs || '').trim() || 'No execution log was returned for this command.'
            });

            const shouldWarn = algorithm === 'LINEAR' && (data.comparisons || 0) > 10;
            setWarningMessage(
                shouldWarn
                    ? 'Linear search is doing a high number of comparisons. Consider Binary, BTree, or Hash for larger datasets.'
                    : ''
            );

            const affectedTable = extractTargetTableName(userQuery);
            if (Array.isArray(data.existing_tables) && data.existing_tables.length) {
                await syncExistingTables(data.existing_tables, affectedTable);
            } else {
                await loadExistingTables(affectedTable);
            }
        } catch (error) {
            setErrorMessage(error.message || 'Unable to connect to the backend.');
            setQueryOutput({
                success: false,
                title: 'Query execution failed',
                message: error.message || 'Unable to connect to the backend.',
                logs: 'No execution log available.'
            });
            setTrace([]);
            setAlgorithmAccess({});
            setLockSnapshots([]);
            setWaitForGraph([]);
            setDeadlockRisk(false);
        } finally {
            setLoading(false);
        }
    }

    useEffect(() => {
        loadExistingTables();
    }, []);

    useEffect(() => {
        function onKeyDown(event) {
            if ((event.ctrlKey || event.metaKey) && event.key === 'Enter') {
                runQuery();
            }
        }

        document.addEventListener('keydown', onKeyDown);
        return () => document.removeEventListener('keydown', onKeyDown);
    }, [queryInput, algorithm, benchmarkEnabled]);

    return (
        <div className="app-shell">
            <header className="topbar">
                <div className="brand-block">
                    <div className="brand-title">DBMS Console</div>
                    <div className="brand-subtitle"></div>
                </div>
                <div className="topbar-actions">
                    <button className="docs-button" type="button" onClick={() => setDocumentationOpen(true)}>
                        Documentation
                    </button>
                </div>
            </header>

            {documentationOpen ? <DocumentationPage onClose={() => setDocumentationOpen(false)} /> : null}

            <main className="workspace-grid">
                <section className="editor-column">
                    <QueryPanel
                        queryInput={queryInput}
                        setQueryInput={setQueryInput}
                        algorithm={algorithm}
                        setAlgorithm={setAlgorithm}
                        benchmarkEnabled={benchmarkEnabled}
                        setBenchmarkEnabled={setBenchmarkEnabled}
                        loading={loading}
                        warningMessage={warningMessage}
                        errorMessage={errorMessage}
                        onRunQuery={runQuery}
                    />
                    <DocumentationPanel />
                </section>

                <aside className="telemetry-column">
                    <div className="telemetry-grid telemetry-grid--top">
                        <SummaryPanel metrics={metrics} />
                        <BenchmarkPanel benchmark={benchmark} />
                    </div>

                    <QueryOutputPanel queryOutput={queryOutput} />
                    <ResultsPanel results={results} />

                    <div className="telemetry-grid">
                        <ExplorerPanel
                            tables={tables}
                            selectedTable={selectedTable}
                            onOpenTable={openTable}
                            onDeleteTable={deleteTable}
                        />
                        <SelectedTablePanel
                            selectedTable={selectedTable}
                            selectedTableRows={selectedTableRows}
                            selectedTableMessage={selectedTableMessage}
                        />
                    </div>

                    <ChartPanel benchmark={benchmark} />
                    <div className="observability-stack">
                        <ComparisonHeatmap
                            algorithmAccess={algorithmAccess}
                            selectedTable={selectedTable}
                            selectedTableRows={selectedTableRows}
                        />
                        <ExecutionTimeline trace={trace} />
                        <LockGraphPanel
                            lockSnapshots={lockSnapshots}
                            waitForGraph={waitForGraph}
                            deadlockRisk={deadlockRisk}
                            trace={trace}
                        />
                    </div>
                </aside>
            </main>

            <footer className="app-footer">
                <span className="app-footer-label">Developed by :</span> Shiva , Raj , Ankit , Tejeshwar
            </footer>
        </div>
    );
}

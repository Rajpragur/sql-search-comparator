export default function QueryPanel(props) {
    const {
        queryInput,
        setQueryInput,
        algorithm,
        setAlgorithm,
        benchmarkEnabled,
        setBenchmarkEnabled,
        loading,
        warningMessage,
        errorMessage,
        onRunQuery
    } = props;

    return (
        <section className="panel panel--editor">
            <div className="panel-header panel-header--editor">
                <div>
                    <div className="panel-kicker">Interactive Workspace</div>
                    <h1 className="panel-title">Code Editor</h1>
                </div>
                <div className="editor-toolbar">
                    <button className="action-button action-button--primary" onClick={onRunQuery} disabled={loading}>
                        {loading ? 'COMPILING...' : 'COMPILE'}
                    </button>
                   
                </div>
            </div>

            <div className="editor-shell">
                <div className="editor-gutter">
                    <span>1</span>
                </div>
                <textarea
                    id="queryInput"
                    className="editor-textarea"
                    placeholder="SELECT * FROM books WHERE accession_no = 123 USING BTREE;"
                    value={queryInput}
                    onChange={(event) => setQueryInput(event.target.value)}
                />
                {/* <div className="editor-side-tools" aria-hidden="true">
                    <span className="editor-tool">+</span>
                    <span className="editor-tool">[]</span>
                    <span className="editor-tool">=</span>
                </div> */}
            </div>

            <div className="editor-bottom">
                <div className="editor-input-row">
                    <label className="editor-inline-label" htmlFor="algorithmSelect">Algorithm</label>
                    <select
                        id="algorithmSelect"
                        className="editor-select"
                        value={algorithm}
                        onChange={(event) => setAlgorithm(event.target.value)}
                    >
                        <option value="LINEAR">Linear</option>
                        <option value="BINARY">Binary</option>
                        <option value="BTREE">BTree</option>
                        <option value="HASH">Hash</option>
                    </select>
                </div>

                <div className="editor-input-row editor-input-row--toggle">
                    <label className="editor-inline-label" htmlFor="benchmarkToggle">Benchmark</label>
                    <input
                        className="editor-toggle"
                        type="checkbox"
                        role="switch"
                        id="benchmarkToggle"
                        checked={benchmarkEnabled}
                        onChange={(event) => setBenchmarkEnabled(event.target.checked)}
                    />
                    <span className="editor-hint">Ctrl/Cmd + Enter</span>
                </div>
            </div>

            {warningMessage ? <div className="panel-alert panel-alert--warning">{warningMessage}</div> : null}
            {errorMessage ? <div className="panel-alert panel-alert--danger">{errorMessage}</div> : null}
        </section>
    );
}

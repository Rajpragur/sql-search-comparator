export default function ExplorerPanel({ tables, selectedTable, onOpenTable, onDeleteTable }) {
    return (
        <section className="panel panel--compact">
            <div className="panel-header">
                <div>
                    <div className="panel-kicker"></div>
                    <h2 className="panel-title panel-title--small">Database Explorer</h2>
                </div>
            </div>

            <div className="table-browser-list">
                {tables.length ? tables.map((tableName) => (
                    <button
                        type="button"
                        key={tableName}
                        className={`table-browser-item ${selectedTable === tableName ? 'is-active' : ''}`}
                        onClick={() => onOpenTable(tableName)}
                    >
                        <span className="table-browser-name">{tableName}</span>
                        <span className="table-browser-actions">
                            <span className="mini-chip">Open</span>
                            {tableName !== 'books' ? (
                                <span
                                    className="mini-chip mini-chip--danger"
                                    onClick={(event) => {
                                        event.preventDefault();
                                        event.stopPropagation();
                                        onDeleteTable(tableName);
                                    }}
                                >
                                    Delete
                                </span>
                            ) : null}
                        </span>
                    </button>
                )) : (
                    <div className="empty-panel">No tables available yet.</div>
                )}
            </div>
        </section>
    );
}

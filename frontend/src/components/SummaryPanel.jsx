export default function SummaryPanel({ metrics }) {
    const algorithmLabel = metrics.search_algorithm || 'Waiting';
    const executionTime = Number.isFinite(metrics.execution_time_ms)
        ? metrics.execution_time_ms.toFixed(3)
        : '0.000';

    return (
        <section className="panel panel--compact">
            <div className="panel-header">
                <div>
                    <div className="panel-kicker"></div>
                    <h2 className="panel-title panel-title--small">Execution Stats</h2>
                </div>
                <span className="status-chip">{algorithmLabel}</span>
            </div>

            <div className="metric-grid">
                <div className="metric-card">
                    <div className="metric-label">Execution</div>
                    <div className="metric-value">{executionTime} ms</div>
                </div>
                <div className="metric-card">
                    <div className="metric-label">Comparisons</div>
                    <div className="metric-value">{metrics.comparisons}</div>
                </div>
            </div>
        </section>
    );
}

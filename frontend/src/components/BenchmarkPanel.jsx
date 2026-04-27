export default function BenchmarkPanel({ benchmark }) {
    const benchmarkEntries = Object.entries(benchmark);
    const fastestTime = benchmarkEntries.length ? Math.min(...benchmarkEntries.map(([, value]) => value)) : null;

    return (
        <section className="panel panel--compact">
            <div className="panel-header">
                <div>
                    <div className="panel-kicker"></div>
                    <h2 className="panel-title panel-title--small">Algorithm Timings</h2>
                </div>
            </div>

            <div className="benchmark-list">
                {benchmarkEntries.length ? benchmarkEntries.map(([name, value]) => (
                    <div key={name} className={`benchmark-item ${value === fastestTime ? 'is-fastest' : ''}`}>
                        <span>{name}</span>
                        <span>{Number(value).toFixed(3)} ms</span>
                    </div>
                )) : (
                    <div className="empty-panel">Benchmark disabled for this request.</div>
                )}
            </div>
        </section>
    );
}

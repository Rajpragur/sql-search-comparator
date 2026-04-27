export default function QueryOutputPanel({ queryOutput }) {
    return (
        <section className="panel">
            <div className="panel-header">
                <div>
                    <div className="panel-kicker"></div>
                    <h2 className="panel-title panel-title--small">Command Response</h2>
                </div>
                <span className={`status-chip ${queryOutput.success ? 'status-chip--ok' : 'status-chip--error'}`}>
                    {queryOutput.success ? 'Completed' : 'Failed'}
                </span>
            </div>

            <div className="terminal-output">
                <div className="terminal-title">{queryOutput.title}</div>
                <p className="terminal-message">{queryOutput.message}</p>
                <pre className="terminal-log">{queryOutput.logs}</pre>
            </div>
        </section>
    );
}

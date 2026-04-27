export default function LockGraphPanel({ lockSnapshots, waitForGraph, deadlockRisk, trace }) {
    const lockEvents = (trace || []).filter((event) => event.stage === 'lock' || event.stage === 'cleanup');
    const recentLockEvents = lockEvents.slice(-8);

    return (
        <section className="panel observability-panel">
            <div className="panel-header">
                <div>
                    <div className="panel-kicker">Concurrency View</div>
                    <h2 className="panel-title panel-title--small">Lock Graph</h2>
                </div>
                <span className={`status-chip ${deadlockRisk ? 'status-chip--error' : 'status-chip--ok'}`}>
                    {deadlockRisk ? 'Deadlock Risk' : 'No Deadlock'}
                </span>
            </div>

            <div className={`lock-graph-shell ${lockSnapshots.length ? '' : 'is-compact'}`}>
                {lockSnapshots.length ? lockSnapshots.map((snapshot) => (
                    <div key={snapshot.table_name} className="lock-node">
                        <div className="lock-node-title">{snapshot.table_name}</div>
                        <div className="lock-node-line">X Holder: {snapshot.exclusive_holder === -1 ? 'None' : `Txn ${snapshot.exclusive_holder}`}</div>
                        <div className="lock-node-line">S Holders: {snapshot.shared_holders.length ? snapshot.shared_holders.map((id) => `Txn ${id}`).join(', ') : 'None'}</div>
                        <div className="lock-node-line">Waiting S: {snapshot.waiting_shared.length ? snapshot.waiting_shared.map((id) => `Txn ${id}`).join(', ') : 'None'}</div>
                        <div className="lock-node-line">Waiting X: {snapshot.waiting_exclusive.length ? snapshot.waiting_exclusive.map((id) => `Txn ${id}`).join(', ') : 'None'}</div>
                    </div>
                )) : (
                    <div className="empty-panel empty-panel--compact">No locks are currently held after the query completes.</div>
                )}
            </div>

            <div className="subpanel subpanel--compact">
                <div className="timeline-focus-title">Wait-For Graph</div>
                {waitForGraph.length ? waitForGraph.map((edge, index) => (
                    <div key={`${edge.from_txn_id}-${edge.to_txn_id}-${index}`} className="graph-edge">
                        Txn {edge.from_txn_id} waits on Txn {edge.to_txn_id} for {edge.table_name} ({edge.requested_lock_type})
                    </div>
                )) : (
                    <div className="terminal-message mb-0">No waiting edges were recorded for this query.</div>
                )}
            </div>

            <div className="timeline-list timeline-list--compact">
                {recentLockEvents.length ? recentLockEvents.map((event) => (
                    <div key={event.id} className="timeline-item">
                        <div className="timeline-item-index">{event.id}</div>
                        <div className="timeline-item-body">
                            <div className="timeline-item-message">{event.message}</div>
                            <div className="timeline-item-meta">
                                {event.txn_id >= 0 ? <span>Txn {event.txn_id}</span> : null}
                                {event.table_name ? <span>{event.table_name}</span> : null}
                                {event.lock_type ? <span>{event.lock_type}</span> : null}
                                {event.status ? <span>{event.status}</span> : null}
                            </div>
                        </div>
                    </div>
                )) : (
                    <div className="empty-panel empty-panel--compact">No lock events recorded yet.</div>
                )}
            </div>
        </section>
    );
}

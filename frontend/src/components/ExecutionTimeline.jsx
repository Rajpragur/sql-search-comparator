import { useEffect, useState } from 'react';

export default function ExecutionTimeline({ trace }) {
    const [currentStep, setCurrentStep] = useState(0);
    const [playing, setPlaying] = useState(false);

    useEffect(() => {
        setCurrentStep(trace.length ? 1 : 0);
        setPlaying(false);
    }, [trace]);

    useEffect(() => {
        if (!playing || !trace.length) {
            return undefined;
        }

        const intervalId = window.setInterval(() => {
            setCurrentStep((value) => {
                if (value >= trace.length) {
                    window.clearInterval(intervalId);
                    setPlaying(false);
                    return value;
                }
                return value + 1;
            });
        }, 900);

        return () => window.clearInterval(intervalId);
    }, [playing, trace]);

    const activeEvent = currentStep ? trace[Math.min(currentStep - 1, trace.length - 1)] : null;
    const visibleTrace = trace.slice(0, currentStep || trace.length);

    return (
        <section className="panel observability-panel">
            <div className="panel-header">
                <div>
                    <div className="panel-kicker">Execution Replay</div>
                    <h2 className="panel-title panel-title--small">Timeline</h2>
                </div>
                <span className="status-chip">{trace.length} steps</span>
            </div>

            <div className="timeline-controls">
                <button className="mini-button" type="button" onClick={() => setPlaying((value) => !value)} disabled={!trace.length}>
                    {playing ? 'Pause' : 'Play'}
                </button>
                <button className="mini-button" type="button" onClick={() => setCurrentStep((value) => Math.max(1, value - 1))} disabled={!trace.length}>
                    Prev
                </button>
                <button className="mini-button" type="button" onClick={() => setCurrentStep((value) => Math.min(trace.length, Math.max(1, value + 1)))} disabled={!trace.length}>
                    Next
                </button>
                <button className="mini-button" type="button" onClick={() => { setCurrentStep(trace.length ? 1 : 0); setPlaying(false); }} disabled={!trace.length}>
                    Reset
                </button>
            </div>

            {activeEvent ? (
                <div className="timeline-focus">
                    <div className="timeline-focus-title">Current Step</div>
                    <div className="timeline-focus-message">{activeEvent.message}</div>
                    <div className="timeline-focus-meta">
                        <span>{activeEvent.stage}</span>
                        {activeEvent.algorithm ? <span>{activeEvent.algorithm}</span> : null}
                        {activeEvent.table_name ? <span>{activeEvent.table_name}</span> : null}
                        {activeEvent.lock_type ? <span>{activeEvent.lock_type}</span> : null}
                    </div>
                </div>
            ) : (
                <div className="empty-panel">Run a query to generate execution steps.</div>
            )}

            <div className={`timeline-list ${trace.length ? '' : 'timeline-list--empty'}`}>
                {(visibleTrace.length ? visibleTrace : trace).map((event) => (
                    <div key={event.id} className={`timeline-item ${activeEvent && activeEvent.id === event.id ? 'is-active' : ''}`}>
                        <div className="timeline-item-index">{event.id}</div>
                        <div className="timeline-item-body">
                            <div className="timeline-item-message">{event.message}</div>
                            <div className="timeline-item-meta">
                                <span>{event.stage}</span>
                                {event.type ? <span>{event.type}</span> : null}
                                {event.row_index >= 0 ? <span>row {event.row_index}</span> : null}
                            </div>
                        </div>
                    </div>
                ))}
            </div>
        </section>
    );
}

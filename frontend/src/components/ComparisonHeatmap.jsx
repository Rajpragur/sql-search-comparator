import { Fragment } from 'react';

function touchByRow(touches) {
    const map = new Map();
    for (const touch of touches || []) {
        if (!map.has(touch.row_index)) {
            map.set(touch.row_index, touch);
        } else if (touch.matched) {
            map.set(touch.row_index, touch);
        }
    }
    return map;
}

export default function ComparisonHeatmap({ algorithmAccess, selectedTable, selectedTableRows }) {
    const algorithms = Object.entries(algorithmAccess || {});
    const rows = selectedTableRows || [];
    const rowTouches = Object.fromEntries(algorithms.map(([name, access]) => [name, touchByRow(access.touches)]));

    return (
        <section className={`panel observability-panel ${algorithms.length && rows.length ? '' : 'observability-panel--compact'}`}>
            <div className="panel-header">
                <div>
                    <div className="panel-kicker">Search Introspection</div>
                    <h2 className="panel-title panel-title--small">Comparison Heatmap</h2>
                </div>
                <span className="status-chip">{selectedTable || 'No table'}</span>
            </div>

            {algorithms.length && rows.length ? (
                <div className="heatmap-wrap">
                    <div className="heatmap-grid" style={{ gridTemplateColumns: `160px repeat(${algorithms.length}, minmax(90px, 1fr))` }}>
                        <div className="heatmap-head heatmap-head--stub">Row</div>
                        {algorithms.map(([name]) => (
                            <div key={name} className="heatmap-head">{name}</div>
                        ))}

                        {rows.map((row, rowIndex) => (
                            <Fragment key={`row-${rowIndex}`}>
                                <div key={`label-${rowIndex}`} className="heatmap-row-label">
                                    #{rowIndex} {Object.values(row).slice(0, 2).join(' | ')}
                                </div>
                                {algorithms.map(([name, access]) => {
                                    const touch = rowTouches[name].get(rowIndex);
                                    const className = touch ? (touch.matched ? 'heatmap-cell is-match' : 'heatmap-cell is-touch') : 'heatmap-cell';
                                    const tooltip = touch
                                        ? `${name}: ${touch.detail} (order ${touch.order})`
                                        : `${name}: no row access recorded`;
                                    return (
                                        <div
                                            key={`${name}-${rowIndex}`}
                                            className={className}
                                            title={tooltip}
                                        >
                                            {touch ? touch.order : '-'}
                                        </div>
                                        );
                                })}
                            </Fragment>
                        ))}
                    </div>
                    <div className="heatmap-legend">
                        <span><i className="legend-swatch"></i>Not touched</span>
                        <span><i className="legend-swatch legend-swatch--touch"></i>Compared / indexed</span>
                        <span><i className="legend-swatch legend-swatch--match"></i>Matched</span>
                    </div>
                </div>
            ) : (
                <div className="empty-panel">Run a SELECT query and open its table to compare row-access patterns across algorithms.</div>
            )}
        </section>
    );
}

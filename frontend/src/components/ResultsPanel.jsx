import DataTable from './DataTable.jsx';
import { countLabel } from '../utils.js';

export default function ResultsPanel({ results }) {
    return (
        <section className="panel">
            <div className="panel-header">
                <div>
                    <div className="panel-kicker"></div>
                    <h2 className="panel-title panel-title--small">Matched Records</h2>
                </div>
                <span className="status-chip">{countLabel(results)}</span>
            </div>

            {results.length ? (
                <DataTable rows={results} />
            ) : (
                <div className="empty-panel">No matching rows found for this query.</div>
            )}
        </section>
    );
}

import DataTable from './DataTable.jsx';
import { countLabel } from '../utils.js';

export default function SelectedTablePanel({ selectedTable, selectedTableRows, selectedTableMessage }) {
    const selectedTableCount = selectedTable ? countLabel(selectedTableRows) : '0 rows';

    return (
        <section className="panel panel--compact">
            <div className="panel-header">
                <div>
                    <div className="panel-kicker"></div>
                    <h2 className="panel-title panel-title--small">{selectedTable || 'Open a table'}</h2>
                </div>
                <span className="status-chip">{selectedTableCount}</span>
            </div>

            <div className="subpanel">
                <p className="terminal-message">{selectedTableMessage}</p>
                {selectedTableRows.length ? <DataTable rows={selectedTableRows} /> : null}
            </div>
        </section>
    );
}

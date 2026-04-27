import BenchmarkChart from './BenchmarkChart.jsx';

export default function ChartPanel({ benchmark }) {
    return (
        <section className="panel">
            <div className="panel-header">
                <div>
                    <div className="panel-kicker">Visualization</div>
                    <h2 className="panel-title panel-title--small">Execution Graph</h2>
                </div>
            </div>
            <div className="chart-wrap">
                <BenchmarkChart benchmark={benchmark} />
            </div>
        </section>
    );
}

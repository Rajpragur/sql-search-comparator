import { useEffect, useRef } from 'react';
import Chart from 'chart.js/auto';

export default function BenchmarkChart({ benchmark }) {
    const canvasRef = useRef(null);
    const chartRef = useRef(null);

    useEffect(() => {
        if (!canvasRef.current) {
            return undefined;
        }

        if (chartRef.current) {
            chartRef.current.destroy();
            chartRef.current = null;
        }

        const entries = Object.entries(benchmark || {});
        if (!entries.length) {
            return undefined;
        }

        const ctx = canvasRef.current.getContext('2d');
        chartRef.current = new Chart(ctx, {
            type: 'bar',
            data: {
                labels: entries.map(([label]) => label),
                datasets: [{
                    label: 'Execution Time (ms)',
                    data: entries.map(([, value]) => value),
                    borderRadius: 8,
                    backgroundColor: ['#6f6f6f', '#858585', '#b8983e', '#f2c94c'],
                    borderColor: '#f2c94c',
                    borderWidth: 1
                }]
            },
            options: {
                maintainAspectRatio: false,
                interaction: {
                    mode: 'index',
                    intersect: false
                },
                plugins: {
                    legend: {
                        display: false
                    },
                    tooltip: {
                        callbacks: {
                            label(context) {
                                return `${context.dataset.label}: ${Number(context.parsed.y).toFixed(3)} ms`;
                            }
                        }
                    }
                },
                scales: {
                    x: {
                        ticks: {
                            color: '#d4d4d4'
                        },
                        grid: {
                            color: 'rgba(255, 255, 255, 0.06)'
                        }
                    },
                    y: {
                        beginAtZero: true,
                        ticks: {
                            color: '#d4d4d4',
                            callback(value) {
                                return `${Number(value).toFixed(3)} ms`;
                            }
                        },
                        grid: {
                            color: 'rgba(255, 255, 255, 0.06)'
                        }
                    }
                }
            }
        });

        return () => {
            if (chartRef.current) {
                chartRef.current.destroy();
                chartRef.current = null;
            }
        };
    }, [benchmark]);

    return <canvas ref={canvasRef}></canvas>;
}

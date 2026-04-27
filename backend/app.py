import json
import os
import re
import subprocess
from threading import Lock

from flask import Flask, jsonify, request, send_from_directory

try:
    from flask_cors import CORS
except ImportError: 
    def CORS(_app):
        return _app


BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
FRONTEND_DIR = os.path.join(BASE_DIR, "frontend")
DIST_DIR = os.path.join(FRONTEND_DIR, "dist")
EXE_PATH = os.path.join(BASE_DIR, "dbms", "dbms")

TABLE_PATTERN = re.compile(r"^\s*CREATE\s+TABLE\s+([A-Za-z_][A-Za-z0-9_]*)", re.IGNORECASE)
DROP_PATTERN = re.compile(r"^\s*DROP\s+TABLE\s+([A-Za-z_][A-Za-z0-9_]*)", re.IGNORECASE)
PERSISTABLE_PATTERN = re.compile(r"\b(CREATE\s+TABLE|INSERT\s+INTO)\b", re.IGNORECASE)
BEGIN_PATTERN = re.compile(r"^\s*BEGIN\b", re.IGNORECASE)
COMMIT_PATTERN = re.compile(r"^\s*COMMIT\b", re.IGNORECASE)
ABORT_PATTERN = re.compile(r"^\s*ABORT\b", re.IGNORECASE)

app = Flask(__name__, static_folder=DIST_DIR, static_url_path="")
CORS(app)

state_lock = Lock()
stateful_sequences = []
benchmark_cache = {}


def coerce_value(value):
    if isinstance(value, str) and value.lstrip("-").isdigit():
        return int(value)
    return value


def coerce_rows(rows):
    return [
        {key: coerce_value(value) for key, value in row.items()}
        for row in rows
    ]


def split_statements(sql):
    return [part.strip() for part in sql.split(";") if part.strip()]


def canonicalize_benchmark_query(query):
    normalized = re.sub(r"\s+", " ", query.strip())
    normalized = re.sub(r"\s+USING\s+\w+\s*;?\s*$", "", normalized, flags=re.IGNORECASE)
    return normalized.strip().rstrip(";")


def normalize_trace_sequence(trace):
    normalized = []
    for index, event in enumerate(trace or [], start=1):
        next_event = dict(event)
        next_event["id"] = index
        normalized.append(next_event)
    return normalized


def extract_persistable_statements(query):
    persisted = []
    in_transaction = False
    transaction_buffer = []

    for statement in split_statements(query):
        if BEGIN_PATTERN.match(statement):
            in_transaction = True
            transaction_buffer = []
            continue

        if COMMIT_PATTERN.match(statement):
            if in_transaction:
                persisted.extend(transaction_buffer)
            in_transaction = False
            transaction_buffer = []
            continue

        if ABORT_PATTERN.match(statement):
            in_transaction = False
            transaction_buffer = []
            continue

        if not PERSISTABLE_PATTERN.search(statement):
            continue

        if in_transaction:
            transaction_buffer.append(statement)
        else:
            persisted.append(statement)

    return persisted


def record_successful_query(query):
    statements_to_persist = extract_persistable_statements(query)
    with state_lock:
        should_clear_benchmarks = False
        for statement in split_statements(query):
            drop_match = DROP_PATTERN.match(statement)
            if drop_match:
                dropped_table = drop_match.group(1)
                updated_history = []
                for existing in stateful_sequences:
                    create_match = TABLE_PATTERN.match(existing)
                    if create_match and create_match.group(1) == dropped_table:
                        continue
                    if re.search(rf"\b(?:INTO|FROM)\s+{re.escape(dropped_table)}\b", existing, re.IGNORECASE):
                        continue
                    updated_history.append(existing)
                stateful_sequences[:] = updated_history
                should_clear_benchmarks = True
            elif PERSISTABLE_PATTERN.search(statement):
                should_clear_benchmarks = True
        if statements_to_persist:
            stateful_sequences.extend(statements_to_persist)
        if should_clear_benchmarks:
            benchmark_cache.clear()


def build_query_with_history(query):
    with state_lock:
        history = list(stateful_sequences)
    if not history:
        return query
    history_with_terminators = [statement if statement.endswith(";") else f"{statement};" for statement in history]
    current_query = query if query.endswith(";") else f"{query};"
    return " ".join(history_with_terminators + [current_query])


def extract_table_names():
    table_names = {"books"}
    with state_lock:
        history = list(stateful_sequences)
    for statement in history:
        match = TABLE_PATTERN.match(statement)
        if match:
            table_names.add(match.group(1))
    return sorted(table_names)


def run_dbms_query(query, benchmark=False, include_history=True):
    effective_query = build_query_with_history(query) if include_history else query
    cmd = [EXE_PATH, "--json", effective_query]
    if benchmark:
        cmd.append("--benchmark")

    result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
    payload = json.loads(result.stdout or "{}")
    if result.returncode != 0 or not payload.get("success", False):
        raise RuntimeError(payload.get("message") or result.stderr.strip() or "Query execution failed.")
    return payload


def run_query_with_stable_benchmark(query):
    cache_key = canonicalize_benchmark_query(query)

    with state_lock:
        cached_bundle = benchmark_cache.get(cache_key)

    if cached_bundle is None:
        payload = run_dbms_query(query, benchmark=True, include_history=True)
        with state_lock:
            benchmark_cache[cache_key] = {
                "benchmark": dict(payload.get("benchmark", {})),
                "algorithm_access": dict(payload.get("algorithm_access", {})),
                "benchmark_trace": [event for event in payload.get("trace", []) if event.get("stage") == "benchmark"],
            }
        payload["trace"] = normalize_trace_sequence(payload.get("trace", []))
        return payload

    payload = run_dbms_query(query, benchmark=False, include_history=True)
    payload["benchmark"] = dict(cached_bundle.get("benchmark", {}))
    payload["algorithm_access"] = dict(cached_bundle.get("algorithm_access", {}))
    payload["trace"] = normalize_trace_sequence(list(payload.get("trace", [])) + list(cached_bundle.get("benchmark_trace", [])))

    selected_algorithm = payload.get("search_algorithm", "")
    selected_benchmark_value = cached_bundle.get("benchmark", {}).get(selected_algorithm)
    if selected_benchmark_value is not None:
        payload["execution_time_ms"] = selected_benchmark_value

    return payload


@app.route("/")
def serve_index():
    if os.path.exists(os.path.join(DIST_DIR, "index.html")):
        return send_from_directory(DIST_DIR, "index.html")
    return (
        "Frontend dist build not found. Run 'npm install' and 'npm run build' inside /frontend, "
        "or start the Vite dev server with 'npm run dev'.",
        503,
    )


@app.route("/executeQuery", methods=["POST"])
def execute_query():
    data = request.get_json(silent=True) or {}
    query = (data.get("query") or "").strip()
    benchmark = bool(data.get("benchmark", False))

    if not query:
        return jsonify({"error": "No query provided."}), 400

    try:
        payload = run_query_with_stable_benchmark(query) if benchmark else run_dbms_query(query, benchmark=False, include_history=True)
        record_successful_query(query)
        payload["results"] = coerce_rows(payload.get("results", []))
        payload["trace"] = normalize_trace_sequence(payload.get("trace", []))
        payload["existing_tables"] = extract_table_names()
        return jsonify(payload)
    except subprocess.TimeoutExpired:
        return jsonify({"error": "Query timeout bounds exceeded."}), 504
    except json.JSONDecodeError:
        return jsonify({"error": "DBMS returned an invalid response."}), 500
    except Exception as exc:
        return jsonify({"error": str(exc)}), 500


@app.route("/tables", methods=["GET"])
def list_tables():
    return jsonify({"tables": extract_table_names()})


@app.route("/tables/<table_name>", methods=["GET"])
def get_table_data(table_name):
    try:
        payload = run_dbms_query(f"SELECT * FROM {table_name};", benchmark=False, include_history=True)
        payload["results"] = coerce_rows(payload.get("results", []))
        payload["trace"] = normalize_trace_sequence(payload.get("trace", []))
        payload["table_name"] = table_name
        return jsonify(payload)
    except Exception as exc:
        return jsonify({"error": str(exc)}), 404


@app.route("/tables/<table_name>", methods=["DELETE"])
def delete_table(table_name):
    try:
        payload = run_dbms_query(f"DROP TABLE {table_name};", benchmark=False, include_history=True)
        record_successful_query(f"DROP TABLE {table_name};")
        payload["existing_tables"] = extract_table_names()
        return jsonify(payload)
    except Exception as exc:
        return jsonify({"error": str(exc)}), 400


@app.route("/query", methods=["POST"])
def legacy_query():
    data = request.get_json(silent=True) or {}
    query = (data.get("query") or "").strip()
    if not query:
        return jsonify({"success": False, "error": "No query provided"}), 400

    try:
        payload = run_dbms_query(query, benchmark=False, include_history=True)
        record_successful_query(query)
        rows = payload.get("results", [])
        columns = list(rows[0].keys()) if rows else []
        table_rows = [[row.get(col, "") for col in columns] for row in rows]
        return jsonify({
            "success": True,
            "output": payload.get("logs", "").strip() or payload.get("message", ""),
            "json": {"columns": columns, "rows": table_rows} if columns else None
        })
    except Exception as exc:
        return jsonify({"success": False, "error": str(exc)}), 500


if __name__ == "__main__":
    print(f"DBMS Executable Bound: {EXE_PATH}")
    if not os.path.exists(EXE_PATH):
        print("WARNING: dbms executable not found! Please run 'make' inside the /dbms directory first.")
    app.run(port=5001, debug=True)

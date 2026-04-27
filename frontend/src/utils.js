export function getApiBase() {
    return import.meta.env.DEV ? '' : '';
}

export function normalizeQuery(query, algorithm) {
    const trimmed = query.trim();
    if (!trimmed) {
        return '';
    }

    const withoutSemicolon = trimmed.replace(/;+\s*$/, '');
    if (!/^select\b/i.test(withoutSemicolon)) {
        return `${withoutSemicolon};`;
    }

    const withoutUsing = withoutSemicolon.replace(/\s+USING\s+\w+\s*$/i, '');
    return `${withoutUsing} USING ${algorithm};`;
}

export function extractTargetTableName(query) {
    const match = query.match(/(?:CREATE\s+TABLE|INSERT\s+INTO|SELECT\s+\*\s+FROM|FROM|DROP\s+TABLE)\s+([A-Za-z_][A-Za-z0-9_]*)/i);
    return match ? match[1] : null;
}

export function countLabel(rows) {
    return `${rows.length} row${rows.length === 1 ? '' : 's'}`;
}

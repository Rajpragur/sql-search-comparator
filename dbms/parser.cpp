#include "parser.h"
#include <algorithm>
#include <cctype>
#include <sstream>

string Parser::trim(const string &str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (first == string::npos)
    return "";
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, last - first + 1);
}

vector<string> Parser::split(const string &str, char delimiter) {
  vector<string> tokens;
  string token;
  istringstream tokenStream(str);
  while (getline(tokenStream, token, delimiter)) {
    tokens.push_back(trim(token));
  }
  return tokens;
}

SearchAlgorithm Parser::parseAlgorithm(const string &value) {
  string upperValue = trim(value);
  transform(upperValue.begin(), upperValue.end(), upperValue.begin(),
            ::toupper);

  if (upperValue == "BINARY") {
    return SearchAlgorithm::BINARY;
  }
  if (upperValue == "BTREE") {
    return SearchAlgorithm::BTREE;
  }
  if (upperValue == "HASH") {
    return SearchAlgorithm::HASH;
  }
  return SearchAlgorithm::LINEAR;
}

Query Parser::parse(const string &sql) {
  Query query;

  string trimmedSql = trim(sql);
  if (trimmedSql.empty())
    return query;

  size_t spaceIndex = trimmedSql.find(' ');

  string firstWord = (spaceIndex != string::npos)
                         ? trimmedSql.substr(0, spaceIndex)
                         : trimmedSql;
  string upperFirstWord = firstWord;
  transform(upperFirstWord.begin(), upperFirstWord.end(),
            upperFirstWord.begin(), ::toupper);

  if (upperFirstWord == "CREATE") {
    query.type = QueryType::CREATE;

    // expected format: CREATE TABLE table_name (col1 INT, col2 STRING)
    string upperSql = trimmedSql;
    transform(upperSql.begin(), upperSql.end(), upperSql.begin(), ::toupper);

    size_t tableKeywordIndex = upperSql.find("TABLE ", spaceIndex + 1);
    if (tableKeywordIndex == string::npos)
      return query;

    size_t nameStart =
        trimmedSql.find_first_not_of(" \t", tableKeywordIndex + 6);
    size_t nameEnd = trimmedSql.find_first_of(" \t(", nameStart);
    if (nameStart == string::npos)
      return query;

    if (nameEnd == string::npos) {
      nameEnd = trimmedSql.find_first_of(";", nameStart);
      if (nameEnd == string::npos) {
        nameEnd = trimmedSql.size();
      }
    }

    query.tableName = trimmedSql.substr(nameStart, nameEnd - nameStart);

    size_t parenStart = trimmedSql.find('(', nameEnd);
    size_t parenEnd = trimmedSql.find_last_of(')');

    if (parenStart != string::npos && parenEnd != string::npos &&
        parenEnd > parenStart) {
      string colsStr =
          trimmedSql.substr(parenStart + 1, parenEnd - parenStart - 1);
      vector<string> colDefsStr = split(colsStr, ',');
      for (string def : colDefsStr) {
        // def format: "col_name INT"
        vector<string> parts = split(def, ' ');
        if (parts.size() >= 2) {
          string typeStr = parts[1];
          transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::toupper);
          DataType dt = (typeStr == "INT") ? DataType::INT : DataType::STRING;
          query.columns.push_back({parts[0], dt});
        } else if (parts.size() == 1 && !parts[0].empty()) {
          // Default to STRING if type not specified
          query.columns.push_back({parts[0], DataType::STRING});
        }
      }
    }
  } else if (upperFirstWord == "INSERT") {
    query.type = QueryType::INSERT;
    // Expected format: INSERT INTO table_name VALUES (val1, val2)
    string upperSql = trimmedSql;
    transform(upperSql.begin(), upperSql.end(), upperSql.begin(), ::toupper);

    size_t intoKeywordIndex = upperSql.find("INTO ", spaceIndex + 1);
    if (intoKeywordIndex == string::npos)
      return query;

    size_t nameStart =
        trimmedSql.find_first_not_of(" \t", intoKeywordIndex + 5);
    size_t nameEnd = trimmedSql.find_first_of(" \t", nameStart);
    if (nameStart == string::npos)
      return query;

    query.tableName = trimmedSql.substr(nameStart, nameEnd - nameStart);

    size_t parenStart = trimmedSql.find('(', nameEnd);
    size_t parenEnd = trimmedSql.find_last_of(')');

    if (parenStart != string::npos && parenEnd != string::npos &&
        parenEnd > parenStart) {
      string valsStr =
          trimmedSql.substr(parenStart + 1, parenEnd - parenStart - 1);
      query.values = split(valsStr, ',');
    }
  } else if (upperFirstWord == "DROP") {
    query.type = QueryType::DROP;
    string upperSql = trimmedSql;
    transform(upperSql.begin(), upperSql.end(), upperSql.begin(), ::toupper);

    size_t tableKeywordIndex = upperSql.find("TABLE ", spaceIndex + 1);
    if (tableKeywordIndex == string::npos)
      return query;

    size_t nameStart =
        trimmedSql.find_first_not_of(" \t", tableKeywordIndex + 6);
    if (nameStart == string::npos)
      return query;

    size_t nameEnd = trimmedSql.find_first_of(" \t;", nameStart);
    if (nameEnd == string::npos) {
      nameEnd = trimmedSql.size();
    }

    query.tableName = trimmedSql.substr(nameStart, nameEnd - nameStart);
  } else if (upperFirstWord == "SELECT") {
    query.type = QueryType::SELECT;
    // Expected format:
    // SELECT * FROM table_name
    // SELECT * FROM table_name WHERE column = value USING BINARY
    string upperCopy = trimmedSql;
    transform(upperCopy.begin(), upperCopy.end(), upperCopy.begin(), ::toupper);

    size_t fromIndex = upperCopy.find("FROM ");
    if (fromIndex != string::npos) {
      size_t nameStart = trimmedSql.find_first_not_of(" \t", fromIndex + 5);
      if (nameStart != string::npos) {
        size_t whereIndex = upperCopy.find(" WHERE ", nameStart);
        size_t usingIndex = upperCopy.find(" USING ", nameStart);

        size_t tableEnd = trimmedSql.find_first_of(" \t;", nameStart);
        if (whereIndex != string::npos) {
          tableEnd = whereIndex;
        } else if (usingIndex != string::npos) {
          tableEnd = usingIndex;
        }

        if (tableEnd == string::npos) {
          tableEnd = trimmedSql.find_last_not_of(" \t;");
          if (tableEnd != string::npos) {
            tableEnd += 1;
          } else {
            tableEnd = trimmedSql.size();
          }
        }

        query.tableName =
            trim(trimmedSql.substr(nameStart, tableEnd - nameStart));
      }
    }

    size_t usingIndex = upperCopy.find(" USING ");
    if (usingIndex != string::npos) {
      string algorithmToken = trimmedSql.substr(usingIndex + 7);
      size_t end = algorithmToken.find(';');
      if (end != string::npos) {
        algorithmToken = algorithmToken.substr(0, end);
      }
      query.searchAlgorithm = parseAlgorithm(algorithmToken);
    }

    size_t whereIndex = upperCopy.find(" WHERE ");
    if (whereIndex != string::npos) {
      size_t clauseStart = whereIndex + 7;
      size_t clauseEnd = (usingIndex != string::npos)
                             ? usingIndex
                             : trimmedSql.find_last_not_of(" \t;");
      if (clauseEnd == string::npos) {
        clauseEnd = trimmedSql.size();
      }
      string whereClause =
          trim(trimmedSql.substr(clauseStart, clauseEnd - clauseStart));
      size_t eqIndex = whereClause.find('=');
      if (eqIndex != string::npos) {
        query.whereColumn = trim(whereClause.substr(0, eqIndex));
        query.whereValue = trim(whereClause.substr(eqIndex + 1));

        if (query.whereValue.size() >= 2) {
          char firstChar = query.whereValue.front();
          char lastChar = query.whereValue.back();
          if ((firstChar == '\'' && lastChar == '\'') ||
              (firstChar == '"' && lastChar == '"')) {
            query.whereValue =
                query.whereValue.substr(1, query.whereValue.size() - 2);
          }
        }
      }
    }
  } else if (upperFirstWord == "BEGIN" || upperFirstWord == "BEGIN;") {
    query.type = QueryType::BEGIN_TXN;
  } else if (upperFirstWord == "COMMIT" || upperFirstWord == "COMMIT;") {
    query.type = QueryType::COMMIT_TXN;
  } else if (upperFirstWord == "ABORT" || upperFirstWord == "ABORT;") {
    query.type = QueryType::ABORT_TXN;
  }

  return query;
}

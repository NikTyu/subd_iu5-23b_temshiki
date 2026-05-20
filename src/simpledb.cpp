#include "simpledb.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace std;
namespace fs = std::filesystem;

namespace simpledb {

static string trim(string s) {
    while (!s.empty() && isspace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && isspace((unsigned char)s.back())) s.pop_back();
    return s;
}

static string upper(string s) {
    for (char& c : s) c = (char)toupper((unsigned char)c);
    return s;
}

static bool startsWithIgnoreCase(const string& s, const string& prefix) {
    return upper(s).rfind(upper(prefix), 0) == 0;
}

static vector<string> splitCSV(const string& text) {
    vector<string> out;
    string cur;
    bool inStr = false;
    for (char c : text) {
        if (c == '\'') inStr = !inStr;
        if (c == ',' && !inStr) {
            out.push_back(trim(cur));
            cur.clear();
        } else cur += c;
    }
    out.push_back(trim(cur));
    return out;
}

static string unquote(string v) {
    v = trim(v);
    if (v.size() >= 2 && v.front() == '\'' && v.back() == '\'')
        return v.substr(1, v.size() - 2);
    return v;
}

static string tablePath(const string& dir, const string& db, const string& table) {
    return dir + "/" + db + "/" + table + ".tbl";
}

static vector<string> readLines(const string& path) {
    ifstream f(path);
    vector<string> lines;
    string line;
    while (getline(f, line)) lines.push_back(line);
    return lines;
}

static vector<string> getColumns(const string& path) {
    auto lines = readLines(path);
    if (lines.empty()) return {};
    return splitCSV(lines[0]);
}

static vector<vector<string>> readRows(const string& path) {
    auto lines = readLines(path);
    vector<vector<string>> rows;
    for (size_t i = 1; i < lines.size(); ++i) rows.push_back(splitCSV(lines[i]));
    return rows;
}

static void writeTable(const string& path, const vector<string>& cols, const vector<vector<string>>& rows) {
    ofstream f(path, ios::trunc);
    for (size_t i = 0; i < cols.size(); ++i) {
        if (i) f << ",";
        f << cols[i];
    }
    f << "\n";
    for (const auto& r : rows) {
        for (size_t i = 0; i < r.size(); ++i) {
            if (i) f << ",";
            f << r[i];
        }
        f << "\n";
    }
}

static int colIndex(const vector<string>& cols, const string& name) {
    for (size_t i = 0; i < cols.size(); ++i)
        if (upper(cols[i]) == upper(name)) return (int)i;
    return -1;
}

static bool matchWhere(const vector<string>& cols, const vector<string>& row, string where) {
    where = trim(where);
    if (where.empty()) return true;

    vector<string> ops = {">=", "<=", "!=", "=", ">", "<"};
    for (const string& op : ops) {
        size_t p = where.find(op);
        if (p != string::npos) {
            string col = trim(where.substr(0, p));
            string val = unquote(where.substr(p + op.size()));
            int idx = colIndex(cols, col);
            if (idx < 0 || idx >= (int)row.size()) return false;
            string cell = row[idx];
            if (op == "=") return cell == val;
            if (op == "!=") return cell != val;
            double a = stod(cell), b = stod(val);
            if (op == ">") return a > b;
            if (op == "<") return a < b;
            if (op == ">=") return a >= b;
            if (op == "<=") return a <= b;
        }
    }
    return false;
}

Database::Database(const string& storage_dir) : storage_dir_(storage_dir) {
    fs::create_directories(storage_dir_);
}

QueryResult Database::execute(const string& query) {
    try {
        string sql = trim(query);
        if (!sql.empty() && sql.back() == ';') sql.pop_back();
        sql = trim(sql);

        if (startsWithIgnoreCase(sql, "CREATE DATABASE ")) return createDatabase(trim(sql.substr(16)));
        if (startsWithIgnoreCase(sql, "DROP DATABASE ")) return dropDatabase(trim(sql.substr(14)));
        if (startsWithIgnoreCase(sql, "USE ")) { current_db_ = trim(sql.substr(4)); return {true, "Using database: " + current_db_}; }
        if (startsWithIgnoreCase(sql, "CREATE TABLE ")) return createTable(sql);
        if (startsWithIgnoreCase(sql, "DROP TABLE ")) return dropTable(trim(sql.substr(11)));
        if (startsWithIgnoreCase(sql, "INSERT INTO ")) return insertInto(sql);
        if (startsWithIgnoreCase(sql, "SELECT ")) return selectFrom(sql);
        if (startsWithIgnoreCase(sql, "UPDATE ")) return updateTable(sql);
        if (startsWithIgnoreCase(sql, "DELETE FROM ")) return deleteFrom(sql);

        return {false, "Unknown command"};
    } catch (const exception& e) {
        return {false, string("Error: ") + e.what()};
    }
}

QueryResult Database::createDatabase(const string& name) {
    fs::create_directories(storage_dir_ + "/" + name);
    current_db_ = name;
    return {true, "Database created: " + name};
}

QueryResult Database::dropDatabase(const string& name) {
    fs::remove_all(storage_dir_ + "/" + name);
    if (current_db_ == name) current_db_.clear();
    return {true, "Database dropped: " + name};
}

QueryResult Database::createTable(const string& sql) {
    if (current_db_.empty()) return {false, "Choose database first: USE dbname;"};
    size_t lp = sql.find('('), rp = sql.rfind(')');
    string name = trim(sql.substr(12, lp - 12));
    string defs = sql.substr(lp + 1, rp - lp - 1);
    vector<string> cols;
    for (auto& d : splitCSV(defs)) {
        stringstream ss(d);
        string col, type;
        ss >> col >> type;
        if (col.empty() || type.empty()) return {false, "Bad column definition"};
        cols.push_back(col);
    }
    writeTable(tablePath(storage_dir_, current_db_, name), cols, {});
    return {true, "Table created: " + name};
}

QueryResult Database::dropTable(const string& name) {
    if (current_db_.empty()) return {false, "Choose database first: USE dbname;"};
    fs::remove(tablePath(storage_dir_, current_db_, name));
    return {true, "Table dropped: " + name};
}

QueryResult Database::insertInto(const string& sql) {
    if (current_db_.empty()) return {false, "Choose database first: USE dbname;"};
    string rest = trim(sql.substr(11));
    size_t valuesPos = upper(rest).find(" VALUES ");
    string before = trim(rest.substr(0, valuesPos));
    string values = trim(rest.substr(valuesPos + 8));

    string table = before;
    vector<string> insertCols;
    size_t lp = before.find('(');
    if (lp != string::npos) {
        table = trim(before.substr(0, lp));
        insertCols = splitCSV(before.substr(lp + 1, before.rfind(')') - lp - 1));
    }

    string path = tablePath(storage_dir_, current_db_, table);
    auto cols = getColumns(path);
    auto rows = readRows(path);
    if (insertCols.empty()) insertCols = cols;

    int affected = 0;
    for (size_t i = 0; i < values.size();) {
        size_t a = values.find('(', i), b = values.find(')', a);
        if (a == string::npos || b == string::npos) break;
        vector<string> vals = splitCSV(values.substr(a + 1, b - a - 1));
        vector<string> row(cols.size(), "NULL");
        for (size_t k = 0; k < vals.size() && k < insertCols.size(); ++k) {
            int idx = colIndex(cols, insertCols[k]);
            if (idx >= 0) row[idx] = unquote(vals[k]);
        }
        rows.push_back(row);
        affected++;
        i = b + 1;
    }
    writeTable(path, cols, rows);
    return {true, "Affected rows: " + to_string(affected), {}, {}, affected};
}

QueryResult Database::selectFrom(const string& sql) {
    size_t fromPos = upper(sql).find(" FROM ");
    string proj = trim(sql.substr(7, fromPos - 7));
    string rest = trim(sql.substr(fromPos + 6));
    size_t wherePos = upper(rest).find(" WHERE ");
    string table = wherePos == string::npos ? rest : trim(rest.substr(0, wherePos));
    string where = wherePos == string::npos ? "" : trim(rest.substr(wherePos + 7));

    string path = tablePath(storage_dir_, current_db_, table);
    auto cols = getColumns(path);
    auto rows = readRows(path);
    vector<string> outCols = (proj == "*") ? cols : splitCSV(proj);
    vector<vector<string>> outRows;

    for (const auto& r : rows) {
        if (!matchWhere(cols, r, where)) continue;
        vector<string> out;
        for (const auto& c : outCols) {
            int idx = colIndex(cols, c);
            out.push_back(idx >= 0 && idx < (int)r.size() ? r[idx] : "NULL");
        }
        outRows.push_back(out);
    }
    return {true, "Selected rows: " + to_string(outRows.size()), outCols, outRows};
}

QueryResult Database::updateTable(const string& sql) {
    string rest = trim(sql.substr(7));
    size_t setPos = upper(rest).find(" SET ");
    string table = trim(rest.substr(0, setPos));
    string afterSet = trim(rest.substr(setPos + 5));
    size_t wherePos = upper(afterSet).find(" WHERE ");
    string assigns = wherePos == string::npos ? afterSet : trim(afterSet.substr(0, wherePos));
    string where = wherePos == string::npos ? "" : trim(afterSet.substr(wherePos + 7));

    string path = tablePath(storage_dir_, current_db_, table);
    auto cols = getColumns(path);
    auto rows = readRows(path);
    int affected = 0;
    auto parts = splitCSV(assigns);

    for (auto& r : rows) {
        if (!matchWhere(cols, r, where)) continue;
        for (auto& a : parts) {
            size_t eq = a.find('=');
            string col = trim(a.substr(0, eq));
            string val = unquote(a.substr(eq + 1));
            int idx = colIndex(cols, col);
            if (idx >= 0) r[idx] = val;
        }
        affected++;
    }
    writeTable(path, cols, rows);
    return {true, "Affected rows: " + to_string(affected), {}, {}, affected};
}

QueryResult Database::deleteFrom(const string& sql) {
    string rest = trim(sql.substr(12));
    size_t wherePos = upper(rest).find(" WHERE ");
    string table = wherePos == string::npos ? rest : trim(rest.substr(0, wherePos));
    string where = wherePos == string::npos ? "" : trim(rest.substr(wherePos + 7));

    string path = tablePath(storage_dir_, current_db_, table);
    auto cols = getColumns(path);
    auto rows = readRows(path);
    vector<vector<string>> kept;
    int affected = 0;
    for (const auto& r : rows) {
        if (matchWhere(cols, r, where)) affected++;
        else kept.push_back(r);
    }
    writeTable(path, cols, kept);
    return {true, "Affected rows: " + to_string(affected), {}, {}, affected};
}

}

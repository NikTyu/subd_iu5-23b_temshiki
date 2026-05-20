#include "simpledb.hpp"
#include <iostream>
#include <iomanip>

using namespace std;
using namespace simpledb;

static void printResult(const QueryResult& r) {
    if (!r.ok) {
        cout << r.message << "\n";
        return;
    }

    if (!r.columns.empty()) {
        for (const auto& c : r.columns) cout << setw(15) << c;
        cout << "\n";
        for (size_t i = 0; i < r.columns.size(); ++i) cout << "---------------";
        cout << "\n";
        for (const auto& row : r.rows) {
            for (const auto& cell : row) cout << setw(15) << cell;
            cout << "\n";
        }
    }
    cout << r.message << "\n";
}

int main(int argc, char** argv) {
    string host = argc > 1 ? argv[1] : "127.0.0.1";
    string port = argc > 2 ? argv[2] : "8080";
    cout << "SimpleDB started at " << host << ":" << port << "\n";
    cout << "Type SQL queries. Type EXIT; to quit.\n\n";

    Database db("data");
    string line, sql;
    while (true) {
        cout << "db> ";
        getline(cin, line);
        sql += line + " ";
        if (sql.find(';') == string::npos) continue;
        if (sql == "EXIT; " || sql == "exit; ") break;
        printResult(db.execute(sql));
        sql.clear();
    }
}

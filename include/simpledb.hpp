#pragma once
#include <string>
#include <vector>

namespace simpledb {

struct QueryResult {
    bool ok = true;
    std::string message;
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    int affected_rows = 0;
};

class Database {
public:
    explicit Database(const std::string& storage_dir = "data");
    QueryResult execute(const std::string& sql);

private:
    std::string storage_dir_;
    std::string current_db_;

    QueryResult createDatabase(const std::string& name);
    QueryResult dropDatabase(const std::string& name);
    QueryResult createTable(const std::string& sql);
    QueryResult dropTable(const std::string& name);
    QueryResult insertInto(const std::string& sql);
    QueryResult selectFrom(const std::string& sql);
    QueryResult updateTable(const std::string& sql);
    QueryResult deleteFrom(const std::string& sql);
};

}

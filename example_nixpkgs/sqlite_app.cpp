/**
 * @file sqlite_app.cpp
 * @brief Example application demonstrating SQLite usage with rules_nixpkgs
 *
 * This program demonstrates how rules_nixpkgs can provide system libraries
 * hermetically. Instead of requiring the host system to have libsqlite3
 * installed, we use rules_nixpkgs to provide the sqlite package.
 *
 * @author Bazel rules_nixpkgs example
 * @date 2025
 */

#include <iostream>
#include <string>
#include <sqlite3.h>

/**
 * @brief Callback function for SQLite query results
 * 
 * @param data Pointer to data passed to the callback
 * @param argc Number of columns in the result
 * @param argv Array of column values as strings
 * @param column_names Array of column names
 * @return 0 on success, non-zero on error
 */
static int callback(void* data, int argc, char** argv, char** column_names) {
    std::cout << "Row: ";
    for (int i = 0; i < argc; i++) {
        std::cout << column_names[i] << " = " << (argv[i] ? argv[i] : "NULL");
        if (i < argc - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    return 0;
}

/**
 * @brief Execute a SQL statement
 * 
 * @param db SQLite database handle
 * @param sql SQL statement to execute
 * @return true on success, false on failure
 */
bool execute_sql(sqlite3* db, const std::string& sql) {
    char* error_message = nullptr;
    int result = sqlite3_exec(db, sql.c_str(), callback, nullptr, &error_message);
    
    if (result != SQLITE_OK) {
        std::cerr << "SQL error: " << error_message << std::endl;
        sqlite3_free(error_message);
        return false;
    }
    return true;
}

/**
 * @brief Main entry point demonstrating SQLite usage
 * 
 * Creates an in-memory SQLite database, creates a table, inserts sample data,
 * and queries the results. This demonstrates that SQLite is working correctly
 * when provided via rules_nixpkgs.
 * 
 * @return 0 on success, 1 on failure
 */
int main() {
    sqlite3* db = nullptr;
    int result;

    std::cout << "SQLite version: " << sqlite3_libversion() << std::endl;
    std::cout << "Starting SQLite example with rules_nixpkgs..." << std::endl;

    // Open an in-memory database
    result = sqlite3_open(":memory:", &db);
    if (result != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    std::cout << "✓ Database opened successfully" << std::endl;

    // Create a simple table
    const std::string create_table_sql = R"(
        CREATE TABLE users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT NOT NULL
        );
    )";

    if (!execute_sql(db, create_table_sql)) {
        sqlite3_close(db);
        return 1;
    }

    std::cout << "✓ Table created successfully" << std::endl;

    // Insert sample data
    const std::string insert_sql = R"(
        INSERT INTO users (name, email) VALUES 
        ('Alice Johnson', 'alice@example.com'),
        ('Bob Smith', 'bob@example.com'),
        ('Carol Davis', 'carol@example.com');
    )";

    if (!execute_sql(db, insert_sql)) {
        sqlite3_close(db);
        return 1;
    }

    std::cout << "✓ Sample data inserted successfully" << std::endl;

    // Query the data
    std::cout << "\nQuerying users table:" << std::endl;
    const std::string select_sql = "SELECT * FROM users;";

    if (!execute_sql(db, select_sql)) {
        sqlite3_close(db);
        return 1;
    }

    // Close the database
    sqlite3_close(db);
    
    std::cout << "\n✓ Database closed successfully" << std::endl;
    std::cout << "SQLite example completed! This demonstrates that rules_nixpkgs" << std::endl;
    std::cout << "successfully provided the SQLite library hermetically." << std::endl;

    return 0;
}